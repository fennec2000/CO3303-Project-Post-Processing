#pragma once
#include <d3dx9math.h>
#include <imgui.h>

D3DXVECTOR4 RGBToHSL(const ImVec4* rgb)
{
	D3DXVECTOR4 hsl; // h.x, s.y, l.z, w.a 

	// set alpha
	hsl.w = rgb->w;

	// conversion
	const float ra = rgb->x;
	const float ga = rgb->y;
	const float ba = rgb->z;
	float Cmax = max(ra, max(ga, ba));
	float Cmin = min(ra, min(ga, ba));
	const float delta = Cmax - Cmin;

	// lightness
	hsl.z = (Cmax + Cmin) / 2;

	// saturation
	if (Cmax == Cmin)
		hsl.y = 0;
	else
		hsl.y = delta / (1 - fabs(2 * hsl.z - 1));

	// hue calc
	if (Cmax == 0)
		hsl.x = 0;
	else if (ra == Cmax)
		hsl.x = (ga - ba) / delta;
	else if (ga == Cmax)
		hsl.x = 2 + (ba - ra) / delta;
	else
		hsl.x = 4 + (ra - ga) / delta;

	// degree conversion
	hsl.x *= 60;
	if (hsl.x < 0)
		hsl.x += 360;

	return hsl;
}

ImVec4 HSLToRBG(const D3DXVECTOR4* hsl)
{
	const float c = (1 - fabs(2 * hsl->z - 1) * hsl->y);
	const float x = c * (1 - fabsf(fmodf(hsl->x / 60, 2.0f) - 1));
	const float m = hsl->z - c / 2;

	float result[3];
	if (hsl->x < 60)		{ result[0] = c; result[1] = x, result[2] = 0; }
	else if (hsl->x < 120)	{ result[0] = x; result[1] = c, result[2] = 0; }
	else if (hsl->x < 180)	{ result[0] = 0; result[1] = c, result[2] = x; }
	else if (hsl->x < 240)	{ result[0] = 0; result[1] = x, result[2] = c; }
	else if (hsl->x < 300)	{ result[0] = x; result[1] = 0, result[2] = c; }
	else					{ result[0] = c; result[1] = 0, result[2] = x; }

	return ImVec4((result[0] + m), (result[1] + m), (result[2] + m), hsl->w);
}