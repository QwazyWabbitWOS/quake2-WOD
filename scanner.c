#include "g_local.h"

void	ClearScanner(gclient_t *client)
{
	client->pers.scanner_active = 0;
}
void Toggle_Scanner (edict_t *ent)
{
	// No scanner if it's been banned.
	if ((int)featureban->value & FB_SCANNER)
		return;

	if (!ent->client || ent->health <= 0)
		return;

	// toggle low on/off bit (and clear scores/inventory display if required)
	if ((ent->client->pers.scanner_active ^= 1) & 1)
	{
		ent->client->showinventory	= 0;
		ent->client->showscores = 0;
	}

	// set "just changed" bit
	ent->client->pers.scanner_active |= 2;
}
void ShowScanner(edict_t *ent,char *layout)
{
	int	i;

	edict_t	*player = g_edicts;

	char	stats[65],
			*tag;

	vec3_t	v;

	// Main scanner graphic draw
	Com_sprintf (stats, sizeof(stats),"xv 80 yv 40 picn %s ", PIC_SCANNER_TAG);
	SAFE_STRCAT(layout,stats,LAYOUT_MAX_LENGTH);

	// Players dots
	for (i=0 ; i < game.maxclients ; i++)
	{
		float	len;

		int		hd;

		// move to player edict
		player++;

		// in use 
		if (!player->inuse
		|| !player->client
		|| player == ent
		|| player->deadflag)
			continue;

		// Don't show ghosts either.
		if (teamplay->value
		&& player->movetype == MOVETYPE_NOCLIP
		&& player->solid == SOLID_NOT)
			continue;

		// calc player to enemy vector
		VectorSubtract (ent->s.origin, player->s.origin, v);

		// save height differential
		hd = v[2] / SCANNER_UNIT;

		// remove height component
		v[2] = 0;

		// calc length of distance from top down view (no z)
		len = VectorLength (v) / SCANNER_UNIT;

		// in range ?
		if (len <= SCANNER_RANGE)
		{
			int		sx,
					sy;

			vec3_t	dp;

			vec3_t	normal = {0,0,-1};


			// normal vector to enemy
			VectorNormalize(v);

			// rotate round player view angle (yaw)
			RotatePointAroundVector( dp, normal, v, ent->s.angles[1]);

			// scale to fit scanner range (80 = pixel range of scanner)
			VectorScale(dp,len*80/SCANNER_RANGE,dp);

			// calc screen (x,y) (2 = half dot width)
			sx = (160 + dp[1]) - 2;
			sy = (120 + dp[0]) - 2;

			// setup dot graphic
			tag = PIC_DOT_TAG;

			if (player->client->quad_framenum > level.framenum)
				tag = PIC_QUADDOT_TAG;

			if (player->client->invincible_framenum > level.framenum)
				tag = PIC_INVDOT_TAG;

			// Set output ...
			Com_sprintf (stats, sizeof(stats),"xv %i yv %i picn %s ",
					sx,
					sy,
					tag);

			SAFE_STRCAT(layout,stats,LAYOUT_MAX_LENGTH);

			// clear stats
			*stats = 0;

			// set up/down arrow
			if (hd < 0)
				Com_sprintf (stats, sizeof(stats),"yv %i picn %s ",
					sy - 5,PIC_UP_TAG);
			else if (hd > 0)
				Com_sprintf (stats, sizeof(stats),"yv %i picn %s ",
					sy + 5,PIC_DOWN_TAG);

			// any up/down ?
			if (*stats)
				SAFE_STRCAT(layout,stats,LAYOUT_MAX_LENGTH);
		}
	}
}
