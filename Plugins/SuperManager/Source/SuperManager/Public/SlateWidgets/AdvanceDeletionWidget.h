// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "CoreMinimal.h"

class SAdvanceDeletionTab : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAdvanceDeletionTab){}
	SLATE_ARGUMENT(FString, TestString);
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
};

/**
 * 
 */
class SUPERMANAGER_API AdvanceDeletionWidget
{
public:
	
};
