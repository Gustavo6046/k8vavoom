//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2002 J�nis Legzdi��
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

// MACROS ------------------------------------------------------------------

// Player macros shared with client

#define MAXNAME			32

#define IT_ALL_MAP		1

#define NUM_CSHIFTS		8

#define SIGNONS		4

//
// Button/action code definitions.
//
#define	BT_ATTACK		1           // Press "Fire".
#define	BT_USE			2           // Use button, to open doors, activate switches.
#define BT_JUMP			4

// TYPES -------------------------------------------------------------------

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
enum psprnum_t
{
    ps_weapon,
    ps_flash,	//	Only DOOM uses it
    NUMPSPRITES
};

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick
struct ticcmd_t
{
	short		forwardmove;	// *2048 for move
	short		sidemove;		// *2048 for move
	short		flymove;		// fly up/down/centering
	byte		buttons;
	byte		impulse;
};

struct model_t;

struct cl_pspdef_t
{
	int			sprite;
	int			frame;
	model_t		*alias_model;
	int			alias_frame;
	int			alias_skinnum;
	float		sx;
	float		sy;
};

enum cactive_t
{
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected		// valid netcon, talking to a server
};

struct client_static_t
{
	cactive_t		state;

	// personalization data sent to server
	char			userinfo[MAX_INFO_STRING];

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	boolean			demorecording;
	boolean			demoplayback;
	boolean			timedemo;
	FArchive		*demofile;
	int				td_lastframe;	// to meter out one message a frame
	int				td_startframe;	// host_framecount at start
	double			td_starttime;	// realtime at second frame of timedemo

	// connection information
	int				signon;			// 0 to SIGNONS
	qsocket_t		*netcon;
	TMessage		message;		// writing buffer to send to server
};

struct client_state_t
{
	int				clientnum;		// cl_mobjs[cl.clientnum] = player

	int				pclass;			// player class type

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
	TVec			vieworg;
	TAVec			viewangles;
	boolean			centering;

    // This is only used between levels,
    // mo->health is used during levels.
    int				health;

	int				items;

    // Frags, kills of other players.
    int				frags[MAXPLAYERS];

    // So gun flashes light up areas.
    int				extralight;

	// For lite-amp and invulnarability powers
    int				fixedcolormap;

    // Current PLAYPAL index
    //  can be set to REDCOLORMAP for pain, etc.
    int				palette;

	dword			cshifts[NUM_CSHIFTS];		//	color shifts for damage,
	dword			prev_cshifts[NUM_CSHIFTS];	// powerups and content types

    // Overlay view sprites (gun, etc).
	cl_pspdef_t		psprites[NUMPSPRITES];
	float			pspriteSY;

    // True if secret level has been done.
    boolean			didsecret;

	dword 			worldTimer;				// total time the player's been playing

	boolean			paused;

    // Overlay view sprites (gun, etc).
	int				translucency;

	int				maxclients;
	int				deathmatch;

	float			time;

	char			serverinfo[MAX_INFO_STRING];

	int				intermission;

	//	128 integers for user defined fields in PROGS
	int				user_fields[128];
#if 0
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame
	usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float	item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;
	
	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset
	
// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start
	
	double		mtime[2];		// the timestamp of last two messages	
	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups
	

	float		last_received_message;	// (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			viewentity;
	int			maxclients;
	int			gametype;

// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	struct efrag_s	*free_efrags;
	int			num_entities;	// held in cl_entities array
	int			num_statics;	// held in cl_staticentities array
	entity_t	viewent;			// the gun model

	int			cdtrack, looptrack;	// cd audio

// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]

#ifdef QUAKE2
// light level at player's position including dlights
// this is sent back to the server each frame
// architectually ugly but it works
	int			light_level;
#endif
#endif
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

boolean CL_Responder(event_t* ev);

// PUBLIC DATA DECLARATIONS ------------------------------------------------

extern client_static_t	cls;
extern client_state_t	cl;
extern TProgs			clpr;

//**************************************************************************
//
//	$Log$
//	Revision 1.8  2002/05/18 16:56:34  dj_jl
//	Added FArchive and FOutputDevice classes.
//
//	Revision 1.7  2002/01/07 12:16:41  dj_jl
//	Changed copyright year
//	
//	Revision 1.6  2001/12/18 19:08:12  dj_jl
//	Beautification
//	
//	Revision 1.5  2001/08/15 17:08:59  dj_jl
//	Fixed finale
//	
//	Revision 1.4  2001/08/07 16:46:23  dj_jl
//	Added player models, skins and weapon
//	
//	Revision 1.3  2001/07/31 17:10:21  dj_jl
//	Localizing demo loop
//	
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
