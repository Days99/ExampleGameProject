// VoiceRoomManager.cpp
// Server-side EOS RTC credential generation for Trusted Server voice chat

#include "VoiceRoomManager.h"
#include "OnlineSubsystem.h"

// EOS C SDK (ships with Unreal under Engine/Source/ThirdParty/EOSSDK)
THIRD_PARTY_INCLUDES_START
#include "eos_sdk.h"
#include "eos_common.h"
#include "eos_rtc_admin.h"
#include "eos_types.h"
THIRD_PARTY_INCLUDES_END

// OnlineSubsystemEOS public interface (UE 5.6+)
#include "IOnlineSubsystemEOS.h"
#include "IEOSSDKManager.h"

namespace
{
	struct FRTCAdminJoinTokenRequest
	{
		TWeakObjectPtr<AVoiceRoomManager> Manager;
		FString ProductUserIdString;
		FString RoomName;
		EOS_HRTCAdmin RTCAdminHandle = nullptr;
		EOS_ProductUserId TargetUserId = nullptr;
	};

	static EOS_HPlatform GetEOSPlatformHandle()
	{
		IOnlineSubsystem* OSS = IOnlineSubsystem::Get(TEXT("EOS"));
		if (!OSS)
		{
			return nullptr;
		}

		// UE 5.6: EOS platform handle is exposed via IOnlineSubsystemEOS::GetEOSPlatformHandle().
		IOnlineSubsystemEOS* EOSSubsystem = static_cast<IOnlineSubsystemEOS*>(OSS);
		if (!EOSSubsystem)
		{
			return nullptr;
		}

		const IEOSPlatformHandlePtr PlatformHandlePtr = EOSSubsystem->GetEOSPlatformHandle();
		if (!PlatformHandlePtr.IsValid())
		{
			return nullptr;
		}

		return static_cast<EOS_HPlatform>(*PlatformHandlePtr);
	}
}

AVoiceRoomManager::AVoiceRoomManager()
{
    PrimaryActorTick.bCanEverTick = true; // Need tick to process EOS callbacks
}

void AVoiceRoomManager::BeginPlay()
{
    Super::BeginPlay();
    
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Ready on server (Trusted Server voice requires RTC Admin credentials)"));
    }
}

void AVoiceRoomManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

	// Ensure EOS callbacks can be dispatched in dedicated server builds.
	if (EOS_HPlatform PlatformHandle = GetEOSPlatformHandle())
	{
		EOS_Platform_Tick(PlatformHandle);
	}
}


void AVoiceRoomManager::AutoAssignMainChannel(const FString& ProductUserId, const FString& MainChannelName)
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Auto-assigning player %s to main channel: %s"), *ProductUserId, *MainChannelName);
    RequestVoiceCredentials(ProductUserId, MainChannelName);
}

void AVoiceRoomManager::RequestVoiceCredentials(const FString& ProductUserId, const FString& RoomName)
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Generating credentials for user %s, room %s"), *ProductUserId, *RoomName);

	EOS_HPlatform PlatformHandle = GetEOSPlatformHandle();
	if (!PlatformHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] EOS Platform handle not available. Is OnlineSubsystemEOS initialized on the server?"));
		FVoiceRoomCredentials Credentials;
		Credentials.RoomName = RoomName;
		Credentials.bIsValid = false;
		OnQueryJoinRoomTokenComplete(ProductUserId, RoomName, false, Credentials);
		return;
	}

	EOS_HRTCAdmin RTCAdminHandle = EOS_Platform_GetRTCAdminInterface(PlatformHandle);
	if (!RTCAdminHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] EOS RTCAdmin interface not available. Server client must have RTC Admin enabled."));
		FVoiceRoomCredentials Credentials;
		Credentials.RoomName = RoomName;
		Credentials.bIsValid = false;
		OnQueryJoinRoomTokenComplete(ProductUserId, RoomName, false, Credentials);
		return;
	}

	// Convert ProductUserId string -> EOS_ProductUserId
	const FTCHARToUTF8 ProductUserIdUtf8(*ProductUserId);
	EOS_ProductUserId TargetUserId = EOS_ProductUserId_FromString(ProductUserIdUtf8.Get());
	if (!TargetUserId)
	{
		UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] Invalid ProductUserId for EOS_ProductUserId_FromString: %s"), *ProductUserId);
		FVoiceRoomCredentials Credentials;
		Credentials.RoomName = RoomName;
		Credentials.bIsValid = false;
		OnQueryJoinRoomTokenComplete(ProductUserId, RoomName, false, Credentials);
		return;
	}

	// Async request context
	FRTCAdminJoinTokenRequest* Req = new FRTCAdminJoinTokenRequest();
	Req->Manager = this;
	Req->ProductUserIdString = ProductUserId;
	Req->RoomName = RoomName;
	Req->RTCAdminHandle = RTCAdminHandle;
	Req->TargetUserId = TargetUserId;

	// Query a join-room token for this user.
	// The callback will provide (or allow copying) the ParticipantToken + ClientBaseUrl.
	const FTCHARToUTF8 RoomNameUtf8(*RoomName);
	EOS_RTCAdmin_QueryJoinRoomTokenOptions Options = {};
	Options.ApiVersion = EOS_RTCADMIN_QUERYJOINROOMTOKEN_API_LATEST;
	// NOTE: EOS requires LocalUserId. For a true "trusted server" setup, the dedicated server must
	// be authenticated in EOS and provide its own ProductUserId here (with RTC Admin permissions).
	// As a minimal default, we set this to the target user; if your server isn't logged in as that user,
	// EOS may return EOS_InvalidParameters / EOS_InvalidUser until server auth is configured.
	Options.LocalUserId = TargetUserId;
	Options.RoomName = RoomNameUtf8.Get();
	Options.TargetUserIds = &TargetUserId;
	Options.TargetUserIdsCount = 1;

	EOS_RTCAdmin_QueryJoinRoomToken(
		RTCAdminHandle,
		&Options,
		Req,
		[](const EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo* Data)
		{
			TUniquePtr<FRTCAdminJoinTokenRequest> Request(static_cast<FRTCAdminJoinTokenRequest*>(Data ? Data->ClientData : nullptr));
			if (!Request.IsValid() || !Request->Manager.IsValid())
			{
				return;
			}

			AVoiceRoomManager* Manager = Request->Manager.Get();
			const FString ProductUserIdStr = Request->ProductUserIdString;
			const FString RoomNameStr = Request->RoomName;

			if (!Data || Data->ResultCode != EOS_EResult::EOS_Success)
			{
				const EOS_EResult Result = Data ? Data->ResultCode : EOS_EResult::EOS_UnexpectedError;
				UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] EOS_RTCAdmin_QueryJoinRoomToken failed for %s room %s (Result=%s)"),
					*ProductUserIdStr, *RoomNameStr, UTF8_TO_TCHAR(EOS_EResult_ToString(Result)));
				FVoiceRoomCredentials Credentials;
				Credentials.RoomName = RoomNameStr;
				Credentials.bIsValid = false;
				Manager->OnQueryJoinRoomTokenComplete(ProductUserIdStr, RoomNameStr, false, Credentials);
				return;
			}

			FString ClientBaseUrl;
			if (Data->ClientBaseUrl)
			{
				ClientBaseUrl = UTF8_TO_TCHAR(Data->ClientBaseUrl);
			}

			// Copy the token for our target user (must be done inside this callback).
			EOS_RTCAdmin_UserToken* UserToken = nullptr;
			EOS_RTCAdmin_CopyUserTokenByUserIdOptions CopyOptions = {};
			CopyOptions.ApiVersion = EOS_RTCADMIN_COPYUSERTOKENBYUSERID_API_LATEST;
			CopyOptions.QueryId = Data->QueryId;
			CopyOptions.TargetUserId = Request->TargetUserId;

			const EOS_EResult CopyResult = EOS_RTCAdmin_CopyUserTokenByUserId(Request->RTCAdminHandle, &CopyOptions, &UserToken);
			if (CopyResult != EOS_EResult::EOS_Success || !UserToken || !UserToken->Token)
			{
				UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] EOS_RTCAdmin_CopyUserTokenByUserId failed (Result=%s) room=%s"),
					UTF8_TO_TCHAR(EOS_EResult_ToString(CopyResult)), *RoomNameStr);
				FVoiceRoomCredentials Credentials;
				Credentials.RoomName = RoomNameStr;
				Credentials.ClientBaseUrl = ClientBaseUrl;
				Credentials.bIsValid = false;
				Manager->OnQueryJoinRoomTokenComplete(ProductUserIdStr, RoomNameStr, false, Credentials);
				return;
			}

			const FString ParticipantToken = UTF8_TO_TCHAR(UserToken->Token);
			EOS_RTCAdmin_UserToken_Release(UserToken);

			if (ClientBaseUrl.IsEmpty() || ParticipantToken.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] QueryJoinRoomToken succeeded but returned empty ClientBaseUrl/ParticipantToken (room %s)"), *RoomNameStr);
				FVoiceRoomCredentials Credentials;
				Credentials.RoomName = RoomNameStr;
				Credentials.ClientBaseUrl = ClientBaseUrl;
				Credentials.bIsValid = false;
				Manager->OnQueryJoinRoomTokenComplete(ProductUserIdStr, RoomNameStr, false, Credentials);
				return;
			}

			// IMPORTANT (UE EOSVoiceChat expects these exact JSON keys):
			//   - "client_base_url"
			//   - "participant_token"
			// Optional:
			//   - "override_userid"
			//
			// Room name is taken from JoinChannel(ChannelName, ...) and is NOT part of the credentials JSON.
			const FString JsonCredentials = FString::Printf(
				TEXT("{\"client_base_url\":\"%s\",\"participant_token\":\"%s\"}"),
				*ClientBaseUrl,
				*ParticipantToken
			);

			FVoiceRoomCredentials Credentials;
			Credentials.RoomName = RoomNameStr;
			Credentials.ClientBaseUrl = ClientBaseUrl;
			Credentials.ParticipantToken = JsonCredentials;
			Credentials.bIsValid = true;

			UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Credentials generated (room=%s, baseUrl=%s, tokenLen=%d)"),
				*RoomNameStr, *ClientBaseUrl, ParticipantToken.Len());

			Manager->OnQueryJoinRoomTokenComplete(ProductUserIdStr, RoomNameStr, true, Credentials);
		}
	);
}

void AVoiceRoomManager::OnQueryJoinRoomTokenComplete(const FString& ProductUserId, const FString& RoomName, bool bSuccess, const FVoiceRoomCredentials& Credentials)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ================================================"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Voice credentials generated successfully"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ProductUserId: %s"), *ProductUserId);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] RoomName: %s"), *RoomName);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ClientBaseUrl: %s"), *Credentials.ClientBaseUrl);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ParticipantToken: %s (len=%d)"),
			Credentials.ParticipantToken.IsEmpty() ? TEXT("None") : TEXT("Provided"),
			Credentials.ParticipantToken.Len());
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ================================================"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Send these credentials to client via RPC"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] Failed to generate credentials for %s"), *ProductUserId);
    }

    // Broadcast to whoever needs to send this to the client
    OnVoiceCredentialsReady.Broadcast(ProductUserId, Credentials);
}
