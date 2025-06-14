/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SSP g_SSP;
SMEXT_LINK(&g_SSP);


IForward* g_hDetect;

CDetour *g_DetourEvents = NULL;
void *DetourAEvents = NULL;
IGameConfig* pGameConfig = nullptr;

bool g_bForwardCalled[SM_MAXPLAYERS + 1] = { false };

DETOUR_DECL_MEMBER1(ListenEvents, bool, CLC_ListenEvents*, msg)
{
	int client = (reinterpret_cast<CBaseClient*>(this))->GetPlayerSlot() + 1;
	
	if (g_bForwardCalled[client])
	{
		return DETOUR_MEMBER_CALL(ListenEvents)(msg);
	}
	g_bForwardCalled[client] = true;
	
	IGamePlayer *pClient = playerhelpers->GetGamePlayer(client);
	
	if (pClient->IsFakeClient()) return DETOUR_MEMBER_CALL(ListenEvents)(msg);
	
	int count = 0;
	
	for (int i = 0; i < MAX_EVENT_NUMBER; i++)
		if (msg->m_EventArray.Get(i))
			count++;
	
	/*
	g_hDetect->PushCell(client);
	g_hDetect->PushCell(count);
	g_hDetect->Execute(NULL);
	*/
	
	const char *name = pClient->GetName();
	smutils->LogMessage(myself, "[SSP_ext] Hook triggered for client %s (%d) with %d events", name, client, count);
	
	return DETOUR_MEMBER_CALL(ListenEvents)(msg);
}

bool SSP::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	CDetourManager::Init(smutils->GetScriptingEngine(), NULL);
	
	Dl_info info;
	if (dladdr((void *)smutils->GetScriptingEngine(), &info) == 0)
	{
		return false;
	}
	
	void *pEngineSo = dlopen(info.dli_fname, RTLD_NOW);
	if (pEngineSo == NULL)
	{
		return false;
	}
	
	DetourAEvents = memutils->ResolveSymbol(pEngineSo,	"_ZN11CBaseClient19ProcessListenEventsEP16CLC_ListenEvents");
	if (!DetourAEvents)
	{
		dlclose(pEngineSo);
		return false;
	}
	dlclose(pEngineSo);
	
	g_DetourEvents = DETOUR_CREATE_MEMBER(ListenEvents, DetourAEvents);
	g_DetourEvents->EnableDetour();
	
	g_hDetect = forwards->CreateForward("SSP_EChecker", ET_Ignore, 2, NULL, Param_Cell, Param_Cell);
	
	sharesys->RegisterLibrary(myself, "SSP_ext");
	
	playerhelpers->AddClientListener(&g_SSP);
	
	return true;
}

void SSP::SDK_OnUnload()
{
	g_DetourEvents->DisableDetour();
	gameconfs->CloseGameConfigFile(pGameConfig);
	forwards->ReleaseForward(g_hDetect);
}

void SSP::OnClientDisconnected(int client)
{
	g_bForwardCalled[client] = false;
}
