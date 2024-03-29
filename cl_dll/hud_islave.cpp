#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "screenfade.h"

DECLARE_MESSAGE(m_ISlave, ISlaveHud)

bool CHudISlave::Init()
{
	HOOK_MESSAGE(ISlaveHud);

	gHUD.AddHudElem(this);
	m_iHealth = 100;

	// gearbox moment: gearbox sets a member variable to zero here
	// which i had initially thought was the hud state
	// but it's not and it never appears anywhere else (other than Reset)
	// it's also not part of CBaseHud, so i believe it's completely unused
	// m_iHudState = 0;

	m_iFlags |= HUD_ACTIVE;

	return true;
}

void CHudISlave::Reset()
{
	m_iFlags |= HUD_ACTIVE;
	// m_iHudState = 0;
	m_iHealth = gHUD.m_Health.m_iHealth;
}

bool CHudISlave::VidInit()
{
	m_iFlags |= HUD_ACTIVE;
	m_HUD_islave_center = gHUD.GetSpriteIndex("islave_center");
	m_HUD_islave_charged = gHUD.GetSpriteIndex("islave_charged");
	m_HUD_islave_inner = gHUD.GetSpriteIndex("islave_inner");
	m_HUD_islave_outer = gHUD.GetSpriteIndex("islave_outer");
	m_HUD_islave_health = gHUD.GetSpriteIndex("islave_health");

	return true;
}

bool CHudISlave::MsgFunc_ISlaveHud(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iHudState = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return true;
}

bool CHudISlave::Draw(float flTime)
{
	// hack to disable hud for c4a1 credits
	if (strstr(gEngfuncs.pfnGetLevelName(), "c4a1"))
	{
		screenfade_t fade;
		gEngfuncs.pfnGetScreenFade(&fade);

		if (fade.fadeEnd != 0 && fade.fadeSpeed < 0 && gEngfuncs.GetClientTime() > fade.fadeEnd)
			return true;
	}

	DrawCrosshair();
	int y = (ScreenHeight - gHUD.m_iFontHeight) - gHUD.m_iFontHeight / 2;

	if (CVAR_GET_FLOAT("cl_alien_hud_scale") >= 1)
		y = ScreenHeight - gHUD.GetSpriteRect(m_HUD_islave_health).bottom;

	m_iHealth = gHUD.m_Health.m_iHealth;
	Rect rect = gHUD.GetSpriteRect(m_HUD_islave_health);
	int x = ScreenWidth / 2 - (rect.right - rect.left) / 2;

	SPR_Set(gHUD.GetSprite(m_HUD_islave_health), 48, 64, 8);
	SPR_DrawAdditive(0, x, y, &rect);

	float middle = rect.right - rect.left;
	float healthPart = middle - m_iHealth * 0.01 * middle;

	if (middle == healthPart)
		return false;

	healthPart *= 0.5;
	rect.right -= healthPart;
	rect.left += healthPart;

	SPR_Set(gHUD.GetSprite(m_HUD_islave_health), 96, 128, 16);
	SPR_DrawAdditive(0, x + healthPart, y, &rect);

	return true;
}

void CHudISlave::DrawCrosshair()
{
	Rect rect;
	int r = 96;
	int g = 128;
	int b = 16;

	switch (m_iHudState)
	{
	case 3:
		r = 255;
		g = 0;
		b = 0;

		rect = gHUD.GetSpriteRect(m_HUD_islave_charged);
		SPR_Set(gHUD.GetSprite(m_HUD_islave_charged), 255, 0, 0);
		SPR_DrawAdditive(0, ScreenWidth / 2 - (rect.right - rect.left) / 2, ScreenHeight / 2 - (rect.bottom - rect.top) / 2, &rect);
	case 1:
		rect = gHUD.GetSpriteRect(m_HUD_islave_inner);
		SPR_Set(gHUD.GetSprite(m_HUD_islave_inner), 96, 128, 16);
		SPR_DrawAdditive(0, ScreenWidth / 2 - (rect.right - rect.left) / 2, (ScreenHeight / 2 - (rect.bottom - rect.top) / 2) + -2, &rect);
	case 2:
		rect = gHUD.GetSpriteRect(m_HUD_islave_outer);
		SPR_Set(gHUD.GetSprite(m_HUD_islave_outer), 96, 128, 16);
		SPR_DrawAdditive(0, ScreenWidth / 2 - (rect.right - rect.left) / 2, (ScreenHeight / 2 - (rect.bottom - rect.top) / 2) + -2, &rect);
	}

	rect = gHUD.GetSpriteRect(m_HUD_islave_center);
	SPR_Set(gHUD.GetSprite(m_HUD_islave_center), r, g, b);
	SPR_DrawAdditive(0, ScreenWidth / 2 - (rect.right - rect.left) / 2, ScreenHeight / 2 - (rect.bottom - rect.top) / 2, &rect);
}