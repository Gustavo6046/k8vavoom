  // cvar functions
  DECLARE_FUNCTION(CvarExists)
  DECLARE_FUNCTION(CreateCvar)
  DECLARE_FUNCTION(GetCvar)
  DECLARE_FUNCTION(SetCvar)
  DECLARE_FUNCTION(GetCvarF)
  DECLARE_FUNCTION(SetCvarF)
  DECLARE_FUNCTION(GetCvarS)
  DECLARE_FUNCTION(SetCvarS)
  DECLARE_FUNCTION(GetCvarB)

  // textures
  DECLARE_FUNCTION(CheckTextureNumForName)
  DECLARE_FUNCTION(TextureNumForName)
  DECLARE_FUNCTION(CheckFlatNumForName)
  DECLARE_FUNCTION(FlatNumForName)
  DECLARE_FUNCTION(TextureHeight)
  DECLARE_FUNCTION(GetTextureName)

  // console command functions
  DECLARE_FUNCTION(Cmd_CheckParm)
  DECLARE_FUNCTION(Cmd_GetArgC)
  DECLARE_FUNCTION(Cmd_GetArgV)
  DECLARE_FUNCTION(CmdBuf_AddText)

  DECLARE_FUNCTION(AreStateSpritesPresent)

  // misc
  DECLARE_FUNCTION(Info_ValueForKey)
  DECLARE_FUNCTION(WadLumpPresent)
  DECLARE_FUNCTION(FindAnimDoor)
  DECLARE_FUNCTION(GetLangString)
  DECLARE_FUNCTION(RGB)
  DECLARE_FUNCTION(RGBA)
  DECLARE_FUNCTION(GetLockDef)
  DECLARE_FUNCTION(ParseColour)
  DECLARE_FUNCTION(TextColourString)
  DECLARE_FUNCTION(StartTitleMap)
  DECLARE_FUNCTION(LoadBinaryLump)
  DECLARE_FUNCTION(IsMapPresent)

#ifdef CLIENT
  DECLARE_FUNCTION(P_GetMapName)
  DECLARE_FUNCTION(P_GetMapIndexByLevelNum)
  DECLARE_FUNCTION(P_GetNumMaps)
  DECLARE_FUNCTION(P_GetMapInfo)
  DECLARE_FUNCTION(P_GetMapLumpName)
  DECLARE_FUNCTION(P_TranslateMap)
  DECLARE_FUNCTION(P_GetNumEpisodes)
  DECLARE_FUNCTION(P_GetEpisodeDef)
  DECLARE_FUNCTION(P_GetNumSkills)
  DECLARE_FUNCTION(P_GetSkillDef)
  DECLARE_FUNCTION(KeyNameForNum)
  DECLARE_FUNCTION(IN_GetBindingKeys)
  DECLARE_FUNCTION(IN_SetBinding)
  DECLARE_FUNCTION(SV_GetSaveString)
  DECLARE_FUNCTION(SV_GetSaveDateString)
  DECLARE_FUNCTION(StartSearch)
  DECLARE_FUNCTION(GetSlist)

  DECLARE_FUNCTION(LoadTextLump)

  // graphics
  DECLARE_FUNCTION(SetVirtualScreen)
  DECLARE_FUNCTION(R_RegisterPic)
  DECLARE_FUNCTION(R_RegisterPicPal)
  DECLARE_FUNCTION(R_GetPicInfo)
  DECLARE_FUNCTION(R_DrawPic)
  DECLARE_FUNCTION(R_InstallSprite)
  DECLARE_FUNCTION(R_DrawSpritePatch)
  DECLARE_FUNCTION(InstallModel)
  DECLARE_FUNCTION(R_DrawModelFrame)
  DECLARE_FUNCTION(R_FillRect)

  // client side sound
  DECLARE_FUNCTION(LocalSound)
  DECLARE_FUNCTION(IsLocalSoundPlaying)
  DECLARE_FUNCTION(StopLocalSounds)

  DECLARE_FUNCTION(TranslateKey)
#endif // CLIENT

#ifdef SERVER
  // map utilites
  DECLARE_FUNCTION(SectorClosestPoint)
  DECLARE_FUNCTION(LineOpenings)
  DECLARE_FUNCTION(P_BoxOnLineSide)
  DECLARE_FUNCTION(FindThingGap)
  DECLARE_FUNCTION(FindOpening)
  DECLARE_FUNCTION(PointInRegion)

  // sound functions
  DECLARE_FUNCTION(SectorStopSound)
  DECLARE_FUNCTION(GetSoundPlayingInfo)
  DECLARE_FUNCTION(GetSoundID)
  DECLARE_FUNCTION(SetSeqTrans)
  DECLARE_FUNCTION(GetSeqTrans)
  DECLARE_FUNCTION(GetSeqSlot)
  DECLARE_FUNCTION(StopAllSounds)

  DECLARE_FUNCTION(SB_Start)
  DECLARE_FUNCTION(TerrainType)
  DECLARE_FUNCTION(GetSplashInfo)
  DECLARE_FUNCTION(GetTerrainInfo)
  DECLARE_FUNCTION(FindClassFromEditorId)
  DECLARE_FUNCTION(FindClassFromScriptId)

  DECLARE_FUNCTION(HasDecal)
#endif // SERVER
