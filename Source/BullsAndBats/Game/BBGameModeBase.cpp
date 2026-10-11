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

void ABBGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ABBPlayerController* PlayerController = Cast<ABBPlayerController>(NewPlayer);
	if (IsValid(PlayerController) == false)
	{
		return;
	}

	ABBPlayerState* PlayerState = PlayerController->GetPlayerState<ABBPlayerState>();
	if (IsValid(PlayerState))
	{
		// 엔진의 기본 플레이어 이름에 접속 순서 번호를 저장합니다.
		PlayerState->SetPlayerName(FString::Printf(TEXT("Player%d"), NextPlayerNumber));
		++NextPlayerNumber;
	}
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

	// 숫자로만 된 입력과 일반 채팅을 먼저 구분합니다. 빈 입력은 숫자로 보지 않습니다.
	bool bIsNumericInput = InChatMessageString.IsEmpty() == false;
	for (TCHAR Character : InChatMessageString)
	{
		if (Character < '0' || Character > '9')
		{
			bIsNumericInput = false;
			break;
		}
	}

	// 일반 채팅에도 보낸 사람의 이름을 붙이고 원문 내용은 유지합니다.
	ABBPlayerState* PlayerState = InChattingPlayerController->GetPlayerState<ABBPlayerState>();
	FString CombinedMessageString = InChatMessageString;
	if (IsValid(PlayerState))
	{
		CombinedMessageString = FString::Printf(TEXT("%s: %s"), *PlayerState->GetPlayerName(), *InChatMessageString);
	}
	if (bIsNumericInput)
	{
		if (IsValid(PlayerState) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("BBPlayerState is missing. Check the GameMode Player State Class."));
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("플레이어 상태를 확인할 수 없습니다. 잠시 후 다시 시도해 주세요."));
			return;
		}

		// 잘못된 숫자 입력에는 기회가 남아 있을 때만 형식을 안내합니다.
		if (IsGuessNumberString(InChatMessageString) == false)
		{
			if (PlayerState->GetCurrentGuessCount() < PlayerState->GetMaxGuessCount())
			{
				InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("숫자 추측은 1~9의 서로 다른 숫자 3개로 입력해 주세요. 예: 123"));
			}
		}
		else
		{
			// 유효한 추측에만 횟수를 사용하고, 소진했다면 본인에게 안내합니다.
			if (PlayerState->TryIncreaseGuessCount() == false)
			{
				InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("추측 횟수를 모두 사용했습니다. 다음 게임을 기다려 주세요."));
				return;
			}

			// 증가한 서버 값으로 메시지를 만들어 첫 추측부터 1/3으로 표시합니다.
			const FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
			CombinedMessageString = FString::Printf(TEXT("%s (%d/%d): %s -> %s"), *PlayerState->GetPlayerName(), PlayerState->GetCurrentGuessCount(), PlayerState->GetMaxGuessCount(), *InChatMessageString, *JudgeResultString);
		}
	}

	// 일반 채팅, 잘못된 숫자 원문, 유효한 추측 결과의 전송 경로를 공유합니다.
	for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
	{
		ABBPlayerController* TargetPlayerController = *It;
		if (IsValid(TargetPlayerController))
		{
			TargetPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
		}
	}
}
