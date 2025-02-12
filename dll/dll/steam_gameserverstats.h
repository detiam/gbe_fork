/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

#include "base.h"
 
//-----------------------------------------------------------------------------
// Purpose: Functions for authenticating users via Steam to play on a game server
//-----------------------------------------------------------------------------
class Steam_GameServerStats : public ISteamGameServerStats
{
    class Settings *settings;
    class Networking *network;
    class SteamCallResults *callback_results;
    class SteamCallBacks *callbacks;
    class RunEveryRunCB *run_every_runcb;
	
	struct RequestAllStats {
		std::chrono::high_resolution_clock::time_point created{};
		SteamAPICall_t steamAPICall{};
		CSteamID steamIDUser{};

		bool timeout = false;
	};

	struct CachedStat {
		bool dirty = false; // true means it was changed on the server and should be sent to the user
		GameServerStats_Messages::StatInfo stat{};
	};
	struct CachedAchievement {
		bool dirty = false; // true means it was changed on the server and should be sent to the user
		GameServerStats_Messages::AchievementInfo ach{};
	};

	struct UserData {
		std::map<std::string, CachedStat> stats{};
		std::map<std::string, CachedAchievement> achievements{};
	};

	std::vector<RequestAllStats> pending_RequestUserStats{};
	std::map<uint64, UserData> all_users_data{};

	CachedStat* find_stat(CSteamID steamIDUser, const std::string &key);
	CachedAchievement* find_ach(CSteamID steamIDUser, const std::string &key);

	void remove_timedout_userstats_requests();
	void collect_and_send_updated_user_stats();
	void steam_run_callback();

	// reponses from player
	void network_callback_initial_stats(Common_Message *msg);
	void network_callback_updated_stats(Common_Message *msg);
	void network_callback(Common_Message *msg);

	// user connect/disconnect
	void network_callback_low_level(Common_Message *msg);

	static void steam_gameserverstats_network_low_level(void *object, Common_Message *msg);
	static void steam_gameserverstats_network_callback(void *object, Common_Message *msg);
	static void steam_gameserverstats_run_every_runcb(void *object);

public:
	Steam_GameServerStats(class Settings *settings, class Networking *network, class SteamCallResults *callback_results, class SteamCallBacks *callbacks, class RunEveryRunCB *run_every_runcb);
	~Steam_GameServerStats();
	
	// downloads stats for the user
	// returns a GSStatsReceived_t callback when completed
	// if the user has no stats, GSStatsReceived_t.m_eResult will be set to k_EResultFail
	// these stats will only be auto-updated for clients playing on the server. For other
	// users you'll need to call RequestUserStats() again to refresh any data
	STEAM_CALL_RESULT( GSStatsReceived_t )
	SteamAPICall_t RequestUserStats( CSteamID steamIDUser );

	// requests stat information for a user, usable after a successful call to RequestUserStats()
	bool GetUserStat( CSteamID steamIDUser, const char *pchName, int32 *pData );
	bool GetUserStat( CSteamID steamIDUser, const char *pchName, float *pData );
	bool GetUserAchievement( CSteamID steamIDUser, const char *pchName, bool *pbAchieved );

	// Set / update stats and achievements. 
	// Note: These updates will work only on stats game servers are allowed to edit and only for 
	// game servers that have been declared as officially controlled by the game creators. 
	// Set the IP range of your official servers on the Steamworks page
	bool SetUserStat( CSteamID steamIDUser, const char *pchName, int32 nData );
	bool SetUserStat( CSteamID steamIDUser, const char *pchName, float fData );
	bool UpdateUserAvgRateStat( CSteamID steamIDUser, const char *pchName, float flCountThisSession, double dSessionLength );

	bool SetUserAchievement( CSteamID steamIDUser, const char *pchName );
	bool ClearUserAchievement( CSteamID steamIDUser, const char *pchName );

	// Store the current data on the server, will get a GSStatsStored_t callback when set.
	//
	// If the callback has a result of k_EResultInvalidParam, one or more stats 
	// uploaded has been rejected, either because they broke constraints
	// or were out of date. In this case the server sends back updated values.
	// The stats should be re-iterated to keep in sync.
	STEAM_CALL_RESULT( GSStatsStored_t )
	SteamAPICall_t StoreUserStats( CSteamID steamIDUser );
};
