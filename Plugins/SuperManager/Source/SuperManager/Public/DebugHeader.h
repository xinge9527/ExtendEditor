#pragma once
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace DebugHeader
{
	inline void Print(const FString& Message, const FColor& Color = FColor::Red)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Message);
		}
	}

	inline void PrintLog(const FString& Message)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}

	inline EAppReturnType::Type ShowMesDialog(EAppMsgType::Type MsgType, const FString& Message, bool bShowMsgWarning)
	{
		if (bShowMsgWarning)
		{
			FText MsgTitle = FText::FromString(TEXT("警告"));
			return FMessageDialog::Open(MsgType, FText::FromString(Message), MsgTitle);
		}else
		{
			return FMessageDialog::Open(MsgType, FText::FromString(Message));
		}
	}

	inline void ShowNotifyInfo(const FString& Message)
	{
		FNotificationInfo NotifyInfo(FText::FromString(Message));
		NotifyInfo.bUseLargeFont = true;
		NotifyInfo.FadeOutDuration = 7.f;

		FSlateNotificationManager::Get().AddNotification(NotifyInfo);
	}

}
