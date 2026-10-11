// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/BBChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/BBPlayerController.h"

void UBBChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯이 다시 표시되더라도 같은 함수가 중복 연결되지 않게 합니다.
	if (IsValid(EditableTextBox_ChatInput)
		&& !EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted))
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UBBChatInput::NativeDestruct()
{
	// 화면에서 제거될 때 이 위젯이 등록한 연결을 해제합니다.
	if (IsValid(EditableTextBox_ChatInput))
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}

	Super::NativeDestruct();
}

void UBBChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// 포커스 이동으로 확정된 입력은 제외하고 Enter 입력만 확인합니다.
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	// 이 위젯을 소유한 컨트롤러를 프로젝트의 컨트롤러 타입으로 확인합니다.
	ABBPlayerController* OwningBBPlayerController = Cast<ABBPlayerController>(GetOwningPlayer());
	if (IsValid(OwningBBPlayerController) == false)
	{
		return;
	}

	// 컨트롤러에 입력 내용을 전달하고, 컨트롤러가 서버 RPC를 호출합니다.
	OwningBBPlayerController->SetChatMessageString(Text.ToString());

	// 처리한 입력 내용을 비웁니다.
	EditableTextBox_ChatInput->SetText(FText::GetEmpty());
}
