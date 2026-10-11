// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BBPlayerController.generated.h"

class UBBChatInput;

/**
 *
 */
UCLASS()
class BULLSANDBATS_API ABBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 위젯에서 전달한 채팅 내용을 받습니다.
	void SetChatMessageString(const FString& InChatMessageString);

	// 이 컨트롤러를 소유한 클라이언트에게 메시지를 전달합니다.
	UFUNCTION(Client, Reliable)
	void ClientRPCPrintChatMessageString(const FString& InChatMessageString);

protected:
	virtual void BeginPlay() override;

	// 소유 클라이언트의 입력 내용을 서버로 전달합니다.
	UFUNCTION(Server, Reliable)
	void ServerRPCPrintChatMessageString(const FString& InChatMessageString);

	// 이 컨트롤러가 마지막으로 전달받은 채팅 내용입니다.
	FString ChatMessageString;

	// 블루프린트에서 화면에 표시할 위젯 클래스를 지정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TSubclassOf<UBBChatInput> ChatInputWidgetClass;

	// 실제로 생성한 위젯 인스턴스를 보관합니다.
	UPROPERTY()
	TObjectPtr<UBBChatInput> ChatInputWidgetInstance;
};
