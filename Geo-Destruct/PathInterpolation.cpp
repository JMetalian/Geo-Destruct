#define OLC_PGE_APPLICATION
#include <iostream>
#include <string>
using namespace std;

#include "olcPixelGameEngine.h"

struct Points
{
	float x;//coordinates
	float y;
};

struct Spline
{
	vector<Points> points;
	Points GetSplinePoint(float t, bool isLooped = false)
	{
		int p0, p1, p2, p3;
		if (!isLooped)
		{
			p1 = (int)t + 1;
			p2 = p1 + 1;
			p3 = p2 + 1;
			p0 = p1 - 1;
		}
		else
		{
			p1 = (int)t;
			p2 = (p1 + 1) % points.size();
			p3 = (p2 + 1) % points.size();
			if (p1 >= 1) 
			{
				p0 = p1 - 1;

			}
			else
			{
				p0 = points.size() - 1;
			}
			//p0 = p1 >= 1 ? p1 - 1 : points.size() - 1;
		}

		t = t - (int)t;

		float tt = t * t;
		float ttt = tt * t;

		float q1 = -ttt + 2.0f * tt - t;
		float q2 = 3.0f * ttt - 5.0f * tt + 2.0f;
		float q3 = -3.0f * ttt + 4.0f * tt + t;
		float q4 = ttt - tt;

		float tx = 0.5f * (points[p0].x * q1 + points[p1].x * q2 + points[p2].x * q3 + points[p3].x * q4);
		float ty = 0.5f * (points[p0].y * q1 + points[p1].y * q2 + points[p2].y * q3 + points[p3].y * q4);

		return{ tx, ty };
	}

	Points GetSplineGradient(float t, bool isLooped = false)
	{
		int p0, p1, p2, p3;
		if (!isLooped)
		{
			p1 = (int)t + 1;
			p2 = p1 + 1;
			p3 = p2 + 1;
			p0 = p1 - 1;
		}
		else
		{
			p1 = (int)t;
			p2 = (p1 + 1) % points.size();
			p3 = (p2 + 1) % points.size();
			p0 = p1 >= 1 ? p1 - 1 : points.size() - 1;
		}

		t = t - (int)t;

		float tt = t * t;
		float ttt = tt * t;

		float q1 = -3.0f * tt + 4.0f * t - 1;
		float q2 = 9.0f * tt - 10.0f * t;
		float q3 = -9.0f * tt + 8.0f * t + 1.0f;
		float q4 = 3.0f * tt - 2.0f * t;

		float tx = 0.5f * (points[p0].x * q1 + points[p1].x * q2 + points[p2].x * q3 + points[p3].x * q4);
		float ty = 0.5f * (points[p0].y * q1 + points[p1].y * q2 + points[p2].y * q3 + points[p3].y * q4);

		return{ tx, ty };
	}
};

class PathInterpolation : public olc::PixelGameEngine
{
public:
	PathInterpolation()
	{
		sAppName = "Path Interpolation";	
	}

private:
	Spline path;
	int selectedPointInPath = 0;
	float fMarker = 0.0f;

protected:
	virtual bool OnUserCreate()
	{
		path.points = { { 10, 41 },{ 20, 41 },{ 30, 41 },{ 40, 41 },{ 50, 41 },{ 60, 41 },{ 70, 41 },{ 80, 41 },{ 90, 41 },{ 100, 41 } };
		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// Clear Screen
		Clear(olc::BLACK);
		// Input Handling
		if (GetKey(olc::Key::X).bReleased)
		{
			selectedPointInPath++;
			if (selectedPointInPath >= path.points.size())
				selectedPointInPath = 0;
		}

		if (GetKey(olc::Key::Z).bReleased)
		{
			selectedPointInPath--;
			if (selectedPointInPath < 0)
				selectedPointInPath = path.points.size() - 1;
		}
		if (GetKey(olc::Key::LEFT).bHeld)
			path.points[selectedPointInPath].x -= 30.0f * fElapsedTime;

		if (GetKey(olc::Key::RIGHT).bHeld)
			path.points[selectedPointInPath].x += 30.0f * fElapsedTime;

		if (GetKey(olc::Key::UP).bHeld)
			path.points[selectedPointInPath].y -= 30.0f * fElapsedTime;

		if (GetKey(olc::Key::DOWN).bHeld)
			path.points[selectedPointInPath].y += 30.0f * fElapsedTime;

		if (GetKey(olc::Key::A).bHeld)
			fMarker -= 5.0f * fElapsedTime;

		if (GetKey(olc::Key::S).bHeld)
			fMarker += 5.0f * fElapsedTime;

		if (fMarker >= (float)path.points.size())
			fMarker -= (float)path.points.size();

		if (fMarker < 0.0f)
			fMarker += (float)path.points.size();

		// Draw Spline
		for (float t = 0; t < (float)path.points.size(); t += 0.005f)
		{
			Points position = path.GetSplinePoint(t, true);
			Draw(position.x, position.y);
		}

		// Draw Control Points
		for (int i = 0; i < path.points.size(); i++)
		{
			DrawLine(path.points[i].x - 1, path.points[i].y - 1, path.points[i].x + 2, path.points[i].y + 2,olc::RED);
			Draw(path.points[i].x, path.points[i].y);
		}

		// Highlight control point
		DrawLine(path.points[selectedPointInPath].x - 1, path.points[selectedPointInPath].y - 1, path.points[selectedPointInPath].x + 2, path.points[selectedPointInPath].y + 2, olc::YELLOW);
		Draw(path.points[selectedPointInPath].x, path.points[selectedPointInPath].y);

		// Draw agent to demonstrate gradient
		Points p1 = path.GetSplinePoint(fMarker, true);
		Points g1 = path.GetSplineGradient(fMarker, true);
		float angle = atan2(-g1.y, g1.x);
		DrawLine((5.0f * sin(angle) + p1.x), (5.0f * cos(angle) + p1.y), (-5.0f * sin(angle) + p1.x), (-5.0f * cos(angle) + p1.y), olc::CYAN);

		return true;
	}
};

int main()
{
	PathInterpolation application;
	application.Construct(100, 80, 10, 10);
	application.Start();
	return 0;
}