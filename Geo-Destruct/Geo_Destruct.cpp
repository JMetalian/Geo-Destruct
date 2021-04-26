#define OLC_PGE_APPLICATION
#include <iostream>
#include <string>
using namespace std;

#include "olcPixelGameEngine.h"


#define ISLOOPED true //Should always be true.
#define VSYNC true
#define FULLSCREEN false

struct Points
{
	//coordinates
	float x;
	float y;
	float distanceBetweenPoints; 
};

struct Spline
{
	vector<Points> points;
	float normOfSpline;
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
		}
		t = t - (int)t;

		float tt = t * t;
		float ttt = tt * t;

		float p1Prime = -ttt + 2.0f * tt - t;
		float p2Prime = 3.0f * ttt - 5.0f * tt + 2.0f;
		float p3Prime = -3.0f * ttt + 4.0f * tt + t;
		float p4Prime = ttt - tt;

		float tx = 0.5f * (points[p0].x * p1Prime + points[p1].x * p2Prime + points[p2].x * p3Prime + points[p3].x * p4Prime);
		float ty = 0.5f * (points[p0].y * p1Prime + points[p1].y * p2Prime + points[p2].y * p3Prime + points[p3].y * p4Prime);

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
			p0 = p1-1;
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

		//Catmull-rom Interpolation
		float p1Prime = -3.0f * tt + 4.0f * t - 1;
		float p2Prime = 9.0f * tt - 10.0f * t;
		float p3Prime = -9.0f * tt + 8.0f * t + 1.0f;
		float p4Prime = 3.0f * tt - 2.0f * t;

		float tx = 0.5f * (points[p0].x * p1Prime + points[p1].x * p2Prime + points[p2].x * p3Prime + points[p3].x * p4Prime);
		float ty = 0.5f * (points[p0].y * p1Prime + points[p1].y * p2Prime + points[p2].y * p3Prime + points[p3].y * p4Prime);

		return{ tx, ty };
	}
	float GetPortionLength(int node, bool isLooped = false) //Returns the norm of single segment among many others between two points
	{
		float norm = 0.0f;
		float stepSize = 0.003f;
		Points oldPoint;
		Points newPoint;

		oldPoint = GetSplinePoint((float)node, isLooped);

		for (float i = 0; i < 1.0f; i += stepSize) 
		{
			newPoint = GetSplinePoint((float)node + i, isLooped);
			//Pythogoras Theorem
			norm += sqrtf((newPoint.x - oldPoint.x) * (newPoint.x - oldPoint.x) + (newPoint.y - oldPoint.y) * (newPoint.y - oldPoint.y));
			oldPoint = newPoint;
		}
		return norm;
	}
	float NormalOffSet(float position)
	{
		int i = 0;
		while (position > points[i].distanceBetweenPoints)
		{
			position -= points[i].distanceBetweenPoints;
			i++;
		}
		return (float)i + (position / points[i].distanceBetweenPoints);
	}
};

class Geo_Destruct : public olc::PixelGameEngine
{
public:
	Geo_Destruct()
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
		path.points = {{ 10, 41 },{ 30, 41 },{ 50, 41 },{ 70, 41 },{ 100, 41 }};
		//for (int i = 0; i < 10; i++)
		//	path.points.push_back({ 30.0f * sinf((float)i / 10.0f * 3.14159f * 2.0f) + ScreenWidth() / 2,
		//							30.0f * cosf((float)i / 10.0f * 3.14159f * 2.0f) + ScreenHeight() / 2 });
		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// Clear Screen
		Clear(olc::BLACK);
		// Input Handling
		//Switch to the previous controlling point by pressing "E" button.
		if (GetKey(olc::Key::E).bReleased)
		{
			selectedPointInPath++;
			if (selectedPointInPath >= path.points.size())
				selectedPointInPath = 0;
		}
		//Switch to the previous controlling point by pressing "Q" button.
		if (GetKey(olc::Key::Q).bReleased)
		{
			selectedPointInPath--;
			if (selectedPointInPath < 0)
				selectedPointInPath = path.points.size() - 1;
		}
		if (GetKey(olc::Key::LEFT).bHeld)//Control the selected point
			path.points[selectedPointInPath].x -= 30.0f * fElapsedTime;

		if (GetKey(olc::Key::RIGHT).bHeld)//Control the selected point
			path.points[selectedPointInPath].x += 30.0f * fElapsedTime;

		if (GetKey(olc::Key::UP).bHeld)//Control the selected point
			path.points[selectedPointInPath].y -= 30.0f * fElapsedTime;

		if (GetKey(olc::Key::DOWN).bHeld)//Control the selected point
			path.points[selectedPointInPath].y += 30.0f * fElapsedTime;

		//Move the character on the spline by pressing "W".
		if (GetKey(olc::Key::W).bHeld)
			fMarker += 50.0f * fElapsedTime;

		if (fMarker >= (float)path.normOfSpline)
			fMarker -= (float)path.normOfSpline;

		if (fMarker < 0.0f)
			fMarker += (float)path.normOfSpline;

		// Draw Spline
		for (float t = 0; t < (float)(path.points.size()-1); t += 0.005f)
		{
			Points position = path.GetSplinePoint(t, ISLOOPED);
			Draw(position.x, position.y);
		} 

		path.normOfSpline = .0f;
		// Draw Control Points
		for (int i = 0; i < path.points.size(); i++)
		{
			path.normOfSpline +=(path.points[i].distanceBetweenPoints = path.GetPortionLength(i, ISLOOPED));
			DrawCircle(path.points[i].x,path.points[i].y, 2, olc::WHITE);
		}

		// Highlight controllers
		DrawLine(path.points[selectedPointInPath].x - 1, path.points[selectedPointInPath].y - 1, path.points[selectedPointInPath].x + 2, path.points[selectedPointInPath].y + 2, olc::RED);
			
		// Gradient demonstrater
		float offSet = path.NormalOffSet(fMarker);
		if (offSet >=(float)(path.points.size() - 1))
		{
			offSet = (float)path.points.size() - 1.0f;
			fMarker-=50.0f*fElapsedTime;
		}
		Points p1 = path.GetSplinePoint(offSet, ISLOOPED);
		Points g1 = path.GetSplineGradient(offSet, ISLOOPED);
		float angle = atan2(-g1.y, g1.x);
		FillCircle(p1.x, p1.y, 3, olc::WHITE);
		DrawLine((3.0f * sin(angle) + p1.x), (3.0f * cos(angle) + p1.y), (-3.0f * sin(angle) + p1.x), (-3.0f * cos(angle) + p1.y), olc::MAGENTA);

		//In order to visualize the
		//DrawString(2, 2, to_string(fMarker),olc::WHITE, 1);
		//DrawString(2, 8, to_string(offSet),olc::WHITE, 1);

		//Place Holder Square Drawings
		//TODO Refine it
		DrawRect(ScreenWidth()/2, ScreenHeight()/2+20, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2+4, ScreenHeight()/2+20, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2, ScreenHeight()/2+24, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2+4, ScreenHeight()/2+24, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2, ScreenHeight()/2+28, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2+4, ScreenHeight()/2+28, 2, 2, olc::WHITE);	
		DrawRect(ScreenWidth()/2, ScreenHeight()/2+32, 2, 2, olc::WHITE);
		DrawRect(ScreenWidth()/2+4, ScreenHeight()/2+32, 2, 2, olc::WHITE);

		return true;
	}
};

int main()
{
	Geo_Destruct application;
	application.Construct(110, 80, 10, 10, FULLSCREEN, VSYNC);
	application.Start();
	return 0;
}