// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/BBGameModeBase.h"
#include "Player/BBPlayerController.h"
#include "Player/BBPlayerState.h"
#include "EngineUtils.h"

void ABBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 서버에서 이번 게임의 정답을 준비합니다.
	SecretNumberString = GenerateSecretNumber();
}

FString ABBGameModeBase::GenerateSecretNumber()
{
	// 0을 제외한 숫자들을 선택 후보로 준비합니다.
	TArray<int32> Numbers;
	for (int32 Number = 1; Number <= 9; ++Number)
	{
		Numbers.Add(Number);
	}

	// 현재 시각을 시드로 사용해 난수 생성기의 시작 상태를 다시 설정합니다.
	FMath::RandInit(FDateTime::Now().GetTicks());

	// 선택한 숫자는 후보에서 제거해 다음 선택에서 중복되지 않게 합니다.
	FString Result;
	for (int32 Count = 0; Count < 3; ++Count)
	{
		const int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	// 디버깅용: 새 정답을 생성할 때마다 서버 로그에 눈에 띄게 출력합니다.
	UE_LOG(LogTemp, Warning, TEXT("[정답 확인] %s"), *Result);

	return Result;
}

bool ABBGameModeBase::IsGuessNumberString(const FString& InNumberString) const
{
	// 입력 원문 전체가 정확히 세 자리여야 합니다.
	if (InNumberString.Len() != 3)
	{
		return false;
	}

	// 허용 범위와 중복을 검사하고, 통과한 문자만 기록합니다.
	TSet<TCHAR> UniqueDigits;
	for (TCHAR Digit : InNumberString)
	{
		if (Digit < TEXT('1') || Digit > TEXT('9'))
		{
			return false;
		}

		if (UniqueDigits.Contains(Digit))
		{
			return false;
		}

		UniqueDigits.Add(Digit);
	}

	return true;
}

FString ABBGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString) const
{
	int32 StrikeCount = 0;
	int32 BallCount = 0;

	// 같은 자리면 스트라이크, 다른 자리에 포함되어 있으면 볼입니다.
	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			++StrikeCount;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				++BallCount;
			}
		}
	}

	// 일치하는 숫자가 없으면 OUT, 있으면 판정 결과 문자열을 만듭니다.
	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS %dB"), StrikeCount, BallCount);
}

void ABBGameModeBase::PrintChatMessageString(ABBPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	if (IsValid(InChattingPlayerController) == false)
	{
		return;
	}

	// 입력 원문 전체가 유효한 추측일 때만 판정 결과를 붙입니다.
	FString CombinedMessageString = InChatMessageString;
	if (IsGuessNumberString(InChatMessageString))
	{
		// 입력한 플레이어의 상태를 가져옵니다. 다른 플레이어의 횟수는 바꾸지 않습니다.
		ABBPlayerState* PlayerState = InChattingPlayerController->GetPlayerState<ABBPlayerState>();
		if (IsValid(PlayerState) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("BBPlayerState is missing. Check the GameMode Player State Class."));
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("플레이어 상태를 확인할 수 없습니다. 잠시 후 다시 시도해 주세요."));
			return;
		}

		// 유효한 추측만 횟수를 사용하며, 한도에 도달하면 판정과 전체 전송을 중단합니다.
		if (PlayerState->TryIncreaseGuessCount() == false)
		{
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("추측 횟수를 모두 사용했습니다. 다음 게임을 기다려 주세요."));
			return;
		}

		// 증가한 서버 값을 사용해 메시지를 완성하므로 첫 추측부터 1/3으로 표시됩니다.
		const FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
		CombinedMessageString = FString::Printf(TEXT("%s (%d/%d): %s -> %s"), *PlayerState->GetPlayerName(), PlayerState->GetCurrentGuessCount(), PlayerState->GetMaxGuessCount(), *InChatMessageString, *JudgeResultString);
	}
	else
	{
		// 일반 채팅은 유지하고, 입력 규칙 안내는 보낸 사람에게만 전달합니다.
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("숫자 추측은 1~9의 서로 다른 숫자 3개로 입력해 주세요. 예: 123"));
	}

	// 서버의 각 컨트롤러를 통해 입력한 본인을 포함한 모두에게 전달합니다.
	for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
	{
		ABBPlayerController* TargetPlayerController = *It;
		if (IsValid(TargetPlayerController))
		{
			TargetPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
		}
	}
}
