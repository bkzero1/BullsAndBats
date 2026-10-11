// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BBPlayerState.generated.h"

/**
 *
 */
UCLASS()
class BULLSANDBATS_API ABBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 다른 클래스는 함수를 통해 횟수를 읽습니다.
	int32 GetCurrentGuessCount() const { return CurrentGuessCount; }
	int32 GetMaxGuessCount() const { return MaxGuessCount; }

	// 서버에서만 남은 횟수를 확인하고 증가시키며, 성공 여부를 반환합니다.
	bool TryIncreaseGuessCount();

	// 새 게임을 시작할 때 서버에서 시도 횟수를 초기화합니다.
	void ResetGuessCount();

protected:
	// 서버가 변경한 현재 횟수를 클라이언트에 복제합니다.
	UPROPERTY(Replicated)
	int32 CurrentGuessCount = 0;

	// 서버와 클라이언트 모두 같은 기본값을 사용하므로 복제하지 않습니다.
	int32 MaxGuessCount = 3;
};
