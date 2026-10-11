// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/BBPlayerController.h"

#include "UI/BBChatInput.h"
#include "BullsAndBats.h"
#include "Engine/World.h"
#include "Game/BBGameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"

void ABBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 이 컴퓨터에서 직접 조작하는 플레이어에게만 UI를 만듭니다.
	if (IsLocalController() == false)
	{
		return;
	}

	// 로컬 플레이어의 공지 위젯을 만들고, 채팅 입력을 가리지 않도록 마우스 판정에서 제외합니다.
	if (IsValid(NotificationTextWidgetClass))
	{
		NotificationTextWidgetInstance = CreateWidget<UUserWidget>(this, NotificationTextWidgetClass);
		if (IsValid(NotificationTextWidgetInstance))
		{
			NotificationTextWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
			NotificationTextWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(ChatInputWidgetClass) == false)
	{
		return;
	}

	// 지정된 위젯 클래스로 인스턴스를 만들고 화면에 표시합니다.
	ChatInputWidgetInstance = CreateWidget<UBBChatInput>(this, ChatInputWidgetClass);
	if (IsValid(ChatInputWidgetInstance) == false)
	{
		return;
	}

	ChatInputWidgetInstance->AddToViewport();

	// 채팅 UI를 마우스로 선택하고 입력할 수 있도록 설정합니다.
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ABBPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 이 컨트롤러의 공지 문구를 소유 클라이언트로 복제합니다.
	DOREPLIFETIME(ThisClass, NotificationText);
}

void ABBPlayerController::SetChatMessageString(const FString& InChatMessageString)
{
	// 입력 내용을 보관하고, 컨트롤러까지 전달됐는지 로그로 확인합니다.
	ChatMessageString = InChatMessageString;
	UE_LOG(LogTemp, Log, TEXT("Controller received chat: %s"), *ChatMessageString);

	// 로컬 플레이어의 입력만 서버 RPC로 전달합니다.
	if (IsLocalController() == true)
	{
		ServerRPCPrintChatMessageString(ChatMessageString);
	}
}

void ABBPlayerController::ServerRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	// 서버의 컨트롤러에서 수신을 확인합니다.
	UE_LOG(LogTemp, Log, TEXT("Server received chat: %s"), *InChatMessageString);

	// 서버의 GameMode에 보낸 사람과 입력 원문을 넘겨 판정을 맡깁니다.
	ABBGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ABBGameModeBase>();
	if (IsValid(GameMode))
	{
		GameMode->PrintChatMessageString(this, InChatMessageString);
	}
}

void ABBPlayerController::ClientRPCPrintChatMessageString_Implementation(const FString& InChatMessageString)
{
	// 수신 로그와 화면 출력을 함께 남겨 전달 여부를 확인합니다.
	UE_LOG(LogTemp, Log, TEXT("Client received chat: %s"), *InChatMessageString);

	BBFunctionLibrary::MyPrintString(this, InChatMessageString, 10.f, FColor::White);
}
