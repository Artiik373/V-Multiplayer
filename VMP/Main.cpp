/* =========================================================
		  V:Multiplayer - http://www.vmultiplayer.com

-- File: Main.cpp
-- Project: Client
-- Author(s): m0niSx
-- Description: Main VMP client source file
=============================================================*/

// NOTES:
/* ------------------- Network ---------------------------------

CNetworkPlayerPool (0x18A8914) 

CIVNetwokPlayerPool class
+ 0x0 - Current number of players
+ 0x8 - Pointer to the first element in the structure
+ 0x14 - Size of CIVNetworkPlayer

CIVNetworkPlayer class
+ 0x18 - Socket handle
+ 0x1C - Network State (4 = Disconnected)
+ 0x38 - Last Tick

Functions:
	- CNetworkPlayer__Process (0x77EEF0) (will keep trying to receive from available client, process network state (pNet + 0x1C), and will handle recv buffers and sockets destroying)
	- GetPlayerNetworkFromPort(?) (0x77E6F0) (will do some tests before returning the correct handle, recommended to use it instead of accessing the ptr directly)
	- CloseSocket(SOCKET socket) (0x77EEB0)
	- Disconnect (0x77EE80) (Same code as CloseSocket, but just with less testing on the pointer validation, use [pNetwork + 4] before calling)
	- VerifySocket (0x77D710) ([edi = socket handle] before calling)
	
	
=============================================================================================

GameNetwork pointer (0x18C7E90)

0x17E49B8 - NetworkState (0xC = disconnected)

CIVGameNetwork class
+ 0xAB4 - pNetworkPlayer[32];
+ 0x2220 - CIVNetworkPlayerManager pointer
+ 0x771C - bIsNetworkConnected

CIVNetworkPlayerManager class
+ 0x4F34 - CIVPlayerInfo *pNetworkPlayers[MAX_PLAYERS]
+ 0x4FB4 (Maximum players or current connected players)

CIVNetworkPlayer class
+ 0x10 - byteGameId
+ 0x3C - State (6 = muted)

Functions:
	- CNetworkPlayerManager__FromHandle (0x54C030) (will return the player network id from handle)

PlayerInfo:
+ 0x30: Network Handle or index (mostly handle)
+ 0x168 - dwTypingFlag (0 not typing, anything else is yes)
+ 0x4DC = PlayerState (6 = left game)

Entity_VFTable:
+ 0x7C : TeleportTo(CVector3 *pPos, float fHeading)

CNetObject:
+ 0xC: Network Id


-----------------------------------------------------------------*/

/* ------------------------- Thread -----------------------------------------

Functions:
	- KillThread (0x5A6950) (ecx = thread handle)
	- GetThreadHandleFromId (0x5A6A10)

*/

/* ------------------------- Viewport -----------------------------------------

CVViewport (size 0x570)
+ 0x53C = Viewport id

Variables:
	- CurrentViewportrsNumber (0x10F47E4)
	- ThreadPool (0x10F47E0)

-------------------------------------- Tasks --------------------------------------

task priority MAX + simple task will not work
trying to set the default primary task when its available will not work cuz the game sets the player's
default task to standby when the player is created

------------------------------------- Vehicles ----------------------------------------

CVehicle + 0xFDC = iCarDoors (car doors count)
CVehicle + 0xFD8 = CCarDoords *

CCarDoords:
struct sDoor (Size 0x34)
{
	CDoor *pDoor;		// 00 - 04
	bool  bIsDamaged;	// 04 - 08
	// Unknown stuff until 0x34
};
	
CarDoors structures (Size 0x34 for each)


------------------------------ CPedIKManager ------------------------------------------

CPed + 0xBC0 = CPedIKManager *

CPedIKManager:

+ 0x70 - 0x80: vecAimTarget (CGTAVector with 4 members)

------------------------------ CWeaponInfo ------------------------------------------

CWeaponInfo:

+ 0x24: Model Hash

0x1540A20 = ARRAY_WeaponInfo

ARRAY_WeaponInfo (Size 0x3C * 0x110) (Max members 0x3C) (each member have a size of 0x110)

------------------------------ CWeapon ------------------------------------------

CWeapon (Size 0x70):

0x18 = Weapon Type


---------------------------- RAGE ---------------------------------------

0x108BFA0 - CRageDirect3D9Device (no pointer) (Size 0x11AC):
0x108D14C - pRageDirect3D9RealDevice (the one that gets created by def funcs)

NOTE: You can either access to real device with its address or like all the 
rage interface func with (CRageDirect3D9Device + 0x11AC)

The class above works as hook interface for the real Direct3D9Device class
it gets copied to the real one and handles everything from there

--------------------------- Loading screen ---------------------------------

Every Item takes a size of 0x110 into the array, so to access a certain item
u'll need to do: ARRAY__ItemType[iItemId * 0x110]

Every item that has multiple objects such as (Texture 1, Texture 2 ...) have a 
maximum of 4 objects and every object takes a size of 0x60. so to access texture 2
for example u'll have to do: 
iSize = iItemId * 0x110
iTexSize = iTexId * 0x60
ARRAY__LoadingScreenTextures[iSize + iTexSize]

Flag Types:
0 = LAST_INGAME
1 = LEGAL
2 = LEGAL 2
3 = START_INTRO
4 = END_INTRO
5 = START_EPISODE
6 = INITIAL_MAIN
7 = WAIT_FOR_AUDIO

Fade Types:
0 = NONE
1 = IN_OUT
2 = IN
3 = OUT

Texture Types:
0 = AFTER
1 = BEFORE


-------------------------- RageRenderVM -------------------------------------

0x10CF680 = pRageRenderVM

CRageRenderVM (Size 0x1D8):

+ 0x1D0: RenderState - 2 = begined / 1 = ended

-------------------------- RenderVM -------------------------------------

0x10F8B00 = pRenderVM



CRenderVM:

+ 0xFB0: hSemaphore
+ 0xFBC: hRenderThread

---------------------- ScreenFade -----------------------------------

CScreenFade (Size 0xC):

+ 0x0: VFTable
+ 0x4: Unknown (initialized at 0)
+ 0x8: pCameraPool

*/





#include "Main.h"

CGame		*pGame;
CConfig		*pConfig;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	// On client load
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		// Disable thread calls
		DisableThreadLibraryCalls(hModule);
		// Are we in the Launcher or the mod itself ?
		if(GetModuleHandle("LaunchGTAIV.exe") != NULL)
		{
			// Install Hooks
			CHooks::InstallHooks();
		}
		else
		{
			// Install the exception handler
			CExceptionHandler::Install();			
			// Create the config instance
			pConfig = new CConfig("VMP/Settings.ini");
			// Init the file
			pConfig->Initialize();
			// Create game class instance
			pGame = new CGame();
			if(!pGame)
			{
				// Log error message
				Log("Failed to create game instance !");
				// Exit the game
				ExitProcess(0);
				return TRUE;
			}
			// Install Hooks
			CHooks::InstallHooks();
			// Initialize the game
			pGame->InitGame();
		}
	}
	else if(dwReason == DLL_PROCESS_DETACH)
	{
		// Delete game instance
		SAFE_DELETE(pGame);
		// Uninstall hooks
		//CHooks::UninstallHooks();
	}
	return TRUE;
}