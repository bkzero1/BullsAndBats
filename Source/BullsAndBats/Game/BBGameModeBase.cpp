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

	// 접속한 플레이어의 공지 위젯에 표시할 문구를 서버에서 설정합니다.
	PlayerController->NotificationText = FText::FromString(TEXT("게임 서버에 접속했습니다."));

	ABBPlayerState* PlayerState = PlayerController->GetPlayerState<ABBPlayerState>();
	if (IsValid(PlayerState))
	{
		// 엔진의 기본 플레이어 이름에 접속 순서 번호를 저장합니다.
		PlayerState->SetPlayerName(FString::Printf(TEXT("Player%d"), NextPlayerNumber));
		++NextPlayerNumber;

		// 디버깅용: 접속 시에도 현재 정답을 다시 출력해 접속 로그 근처에서 확인합니다.
		UE_LOG(LogTemp, Warning, TEXT("[접속 시 정답 확인] %s 접속 / 현재 정답: %s"), *PlayerState->GetPlayerName(), *SecretNumberString);
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

	// 숫자로만 된 입력인지와 숫자 포함 여부를 확인합니다. 문자 뒤의 숫자도 확인하므로 끝까지 순회합니다.
	bool bIsNumericInput = InChatMessageString.IsEmpty() == false;
	bool bHasDigit = false;
	for (TCHAR Character : InChatMessageString)
	{
		if (Character >= '0' && Character <= '9')
		{
			bHasDigit = true;
		}
		else
		{
			bIsNumericInput = false;
		}
	}
	// 세 글자에 숫자와 다른 문자가 섞인 입력도 오류 안내 대상으로 봅니다.
	const bool bIsMixedGuessInput = InChatMessageString.Len() == 3 && bHasDigit && bIsNumericInput == false;

	// 일반 채팅에도 보낸 사람의 이름을 붙이고 원문 내용은 유지합니다.
	ABBPlayerState* PlayerState = InChattingPlayerController->GetPlayerState<ABBPlayerState>();
	FString CombinedMessageString = InChatMessageString;
	FString InputErrorMessage;
	bool bIsGuessAccepted = false;
	if (IsValid(PlayerState))
	{
		CombinedMessageString = FString::Printf(TEXT("%s: %s"), *PlayerState->GetPlayerName(), *InChatMessageString);
	}
	if (bIsNumericInput || bIsMixedGuessInput)
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
				// 오류 문구만 준비하고, 입력 원문을 전송한 뒤 본인에게 안내합니다.
				if (InChatMessageString.Len() != 3)
				{
					InputErrorMessage = TEXT("3자리 숫자를 입력해주세요.");
				}
				else if (bIsNumericInput == false)
				{
					InputErrorMessage = TEXT("숫자만 입력해주세요.");
				}
				else if (InChatMessageString.Contains(TEXT("0")))
				{
					InputErrorMessage = TEXT("1~9 사이의 숫자만 입력해주세요.");
				}
				else
				{
					// 세 자리이고 모두 1~9인데 검사를 통과하지 못했다면 중복이 원인입니다.
					InputErrorMessage = TEXT("중복되지 않은 숫자를 입력해주세요.");
				}
			}
		}
		else
		{
			// 게임이 끝났다면 일반 채팅은 허용하되 새로운 추측은 받지 않습니다.
			if (bIsGameOver)
			{
				InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("게임이 종료되었습니다. 다음 게임을 기다려 주세요."));
				return;
			}

			// 유효한 추측에만 횟수를 사용하고, 소진했다면 본인에게 안내합니다.
			if (PlayerState->TryIncreaseGuessCount() == false)
			{
				InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("추측 횟수를 모두 사용했습니다. 다음 게임을 기다려 주세요."));
				return;
			}

			// 증가한 서버 값으로 메시지를 만들어 첫 추측부터 1/3으로 표시합니다.
			const FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
			bIsGuessAccepted = true;
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

	// 입력 원문을 모두에게 보낸 뒤, 오류 안내는 보낸 사람에게만 전달합니다.
	if (InputErrorMessage.IsEmpty() == false)
	{
		InChattingPlayerController->ClientRPCPrintChatMessageString(InputErrorMessage);
	}

	// 추측 결과를 먼저 전송한 뒤, 실제로 기회를 사용한 입력에만 종료 판정을 합니다.
	if (bIsGuessAccepted)
	{
		// 마지막 기회에 틀렸다면 추가 입력을 기다리지 않고 본인에게 소진 사실을 알립니다.
		if (InChatMessageString != SecretNumberString && PlayerState->GetCurrentGuessCount() >= PlayerState->GetMaxGuessCount())
		{
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("이번 게임의 추측 횟수를 모두 사용했습니다."));
		}

		JudgeGame(InChattingPlayerController, InChatMessageString);
	}
}

void ABBGameModeBase::JudgeGame(ABBPlayerController* InChattingPlayerController, const FString& InGuessNumberString)
{
	if (bIsGameOver || IsValid(InChattingPlayerController) == false)
	{
		return;
	}

	ABBPlayerState* PlayerState = InChattingPlayerController->GetPlayerState<ABBPlayerState>();
	if (IsValid(PlayerState) == false)
	{
		return;
	}

	// 정답과 완전히 같으면 3S입니다. 마지막 기회여도 무승부보다 승리가 우선입니다.
	if (InGuessNumberString == SecretNumberString)
	{
		bIsGameOver = true;
		UE_LOG(LogTemp, Warning, TEXT("[게임 결과] %s 승리!"), *PlayerState->GetPlayerName());

		// 모든 플레이어의 공지 문구를 설정하면 각 소유 클라이언트의 위젯에 복제되어 표시됩니다.
		const FText WinnerNotification = FText::FromString(FString::Printf(TEXT("%s 승리!\n새 게임이 시작되었습니다."), *PlayerState->GetPlayerName()));
		for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
		{
			ABBPlayerController* PlayerController = *It;
			if (IsValid(PlayerController))
			{
				PlayerController->NotificationText = WinnerNotification;
			}
		}
		// 모든 플레이어에게 공지를 설정한 뒤 한 번만 초기화합니다.
		ResetGame();
		return;
	}

	// 접속 중인 플레이어 중 한 명이라도 기회가 남아 있으면 게임을 계속합니다.
	for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
	{
		ABBPlayerController* PlayerController = *It;
		if (IsValid(PlayerController) == false)
		{
			continue;
		}

		ABBPlayerState* OtherPlayerState = PlayerController->GetPlayerState<ABBPlayerState>();
		if (IsValid(OtherPlayerState) == false || OtherPlayerState->GetCurrentGuessCount() < OtherPlayerState->GetMaxGuessCount())
		{
			return;
		}
	}

	// 모두 소진했고 정답자도 없을 때만 무승부로 종료하고 모든 플레이어에게 알립니다.
	bIsGameOver = true;
	UE_LOG(LogTemp, Warning, TEXT("[게임 결과] 무승부"));

	for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
	{
		ABBPlayerController* PlayerController = *It;
		if (IsValid(PlayerController))
		{
			PlayerController->NotificationText = FText::FromString(TEXT("무승부입니다.\n새 게임이 시작되었습니다."));
		}
	}
	ResetGame();
}

void ABBGameModeBase::ResetGame()
{
	// 새 정답을 한 번 생성합니다. 생성 함수가 서버에 정답 로그도 남깁니다.
	SecretNumberString = GenerateSecretNumber();

	for (TActorIterator<ABBPlayerController> It(GetWorld()); It; ++It)
	{
		ABBPlayerController* PlayerController = *It;
		if (IsValid(PlayerController) == false)
		{
			continue;
		}

		ABBPlayerState* PlayerState = PlayerController->GetPlayerState<ABBPlayerState>();
		if (IsValid(PlayerState))
		{
			PlayerState->ResetGuessCount();
		}
	}

	// 공지 문구는 덮어쓰지 않고, 데이터 준비가 끝난 뒤 추측을 다시 허용합니다.
	bIsGameOver = false;
}
