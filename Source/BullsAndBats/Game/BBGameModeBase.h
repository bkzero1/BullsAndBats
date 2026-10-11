// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BBGameModeBase.generated.h"

class ABBPlayerController;

/**
 *
 */
UCLASS()
class BULLSANDBATS_API ABBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 서버에서 입력을 검사하고 판정한 메시지를 플레이어들에게 전달합니다.
	void PrintChatMessageString(ABBPlayerController* InChattingPlayerController, const FString& InChatMessageString);

protected:
	virtual void BeginPlay() override;
	virtual void OnPostLogin(AController* NewPlayer) override;

	// 같은 서버 세션에서 접속할 때마다 다음 번호를 부여합니다.
	int32 NextPlayerNumber = 1;

	// 1~9에서 중복 없이 세 자리를 골라 정답 문자열을 만듭니다.
	FString GenerateSecretNumber();

	// 입력 원문이 중복 없는 1~9의 세 자리 숫자인지 검사합니다.
	bool IsGuessNumberString(const FString& InNumberString) const;

	// 유효한 세 자리 정답과 추측을 비교해 S/B 또는 OUT을 반환합니다.
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString) const;

	// 받아들인 추측 이후 승리를 먼저 확인하고, 모두 기회를 소진했으면 무승부로 종료합니다.
	void JudgeGame(ABBPlayerController* InChattingPlayerController, const FString& InGuessNumberString);

	// 결과 공지는 유지하고 정답과 모든 플레이어의 횟수를 초기화합니다.
	void ResetGame();

	// 종료된 게임에서 추가 추측으로 결과가 바뀌는 것을 막습니다.
	bool bIsGameOver = false;

	// GameMode는 서버에만 존재하므로 정답을 이곳에 보관합니다.
	FString SecretNumberString;
};
