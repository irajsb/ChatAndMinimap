// Copyright 2024 Iraj Mohtasham aurelion.net

#pragma once

#include "Modules/ModuleManager.h"

class FChatSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
