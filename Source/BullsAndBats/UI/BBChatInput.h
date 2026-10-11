// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "BBChatInput.generated.h"

class UEditableTextBox;

/**
 *
 */
UCLASS()
class BULLSANDBATS_API UBBChatInput : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯의 화면 수명에 맞춰 입력 이벤트를 연결하고 해제합니다.
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 입력 확정 이벤트가 전달하는 내용과 확정 방식을 받습니다.
	UFUNCTION()
	void OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	// 블루프린트에 배치한 같은 이름의 입력창과 연결합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> EditableTextBox_ChatInput;
};
