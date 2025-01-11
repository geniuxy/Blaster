// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

//
// Declaring our own custom delegates for the Menu class to bind callbacks to
// MULTICAST: 广播
// 这个DYNAMIC 意味着绑定到这个delegate的callback函数必须是UFUNCTION
// 即，AddDynamic绑定的函数必须有UFUNCTION()注释
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE和DECLARE_MULTICAST_DELEGATE的区别就是：
// 前者可以用于蓝图，而后者可以在参数的类不是U...(蓝图类)时使用
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionDelegate, bool, bWasSuccessful);

DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsDelegate,
                                     const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionDelegate, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionDelegate, bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionDelegate, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();

	//
	//To handle session functionality, the menu will call these
	//
	void CreateSession(int32 NumOfPublicConnections, FString matchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	//
	// Out custom delegates for the Menu to bind callbacks to
	//
	FMultiplayerOnCreateSessionDelegate MultiplayerOnCreateSessionDelegate;
	FMultiplayerOnFindSessionsDelegate MultiplayerOnFindSessionsDelegate;
	FMultiplayerOnJoinSessionDelegate MultiplayerOnJoinSessionDelegate;
	FMultiplayerOnDestroySessionDelegate MultiplayerOnDestroySessionDelegate;
	FMultiplayerOnStartSessionDelegate MultiplayerOnStartSessionDelegate;

protected:
	//
	// Internal callbacks for the delegates
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	//
	// To add the online session interface delegate list
	// we'll bind MultiplayerSubsystem internal callbacks to these
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bDestroySessionOnCreate{false};
	int32 LastNumOfPublicConnections;
	FString LastMatchType;
};
