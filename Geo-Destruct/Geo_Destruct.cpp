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

struct CircleCollider
{
	float px, py;
	float vx, vy;
	float ax, ay;
	float radius;
	float mass;

	int id;
};


class Geo_Destruct : public olc::PixelGameEngine
{
private:
	Spline path;
	int selectedPointInPath = 0;
	float fMarker = 0.0f;

	
	vector<pair<float, float>> defaultCircleCollider;
	vector<CircleCollider> vectorOfAllColliders;
	CircleCollider* pSelectedBall = nullptr;
	// Adds a ball to the vector
	void AddNewCircleCollider(float x, float y, float r = 5.0f)
	{
		CircleCollider circleCollider;
		circleCollider.px = x; circleCollider.py = y;
		circleCollider.vx = 0; circleCollider.vy = 0;
		circleCollider.ax = 0; circleCollider.ay = 0;
		circleCollider.radius = r;
		circleCollider.mass = r * 10.0f;

		circleCollider.id = vectorOfAllColliders.size();//In order to uniquely name them.
		vectorOfAllColliders.emplace_back(circleCollider);
	}

public:
	Geo_Destruct()
	{
		sAppName = "Geo-Destruct";
	}

protected:
	virtual bool OnUserCreate()
	{
		path.points = {{ 10, 41 },{ 30, 41 },{ 50, 41 },{ 70, 41 },{ 100, 41 }};

		//A Default Circle Collider
		defaultCircleCollider.push_back({ 0.0f, 0.0f });
		int nPoints = 20;
		for (int i = 0; i < nPoints; i++)
			defaultCircleCollider.push_back({ cosf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) , sinf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) });

		float radiusForBalls = 3.0f;
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 10, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 10, radiusForBalls);
		AddNewCircleCollider(ScreenWidth()/2, ScreenHeight() / 2 + 14, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2+6, ScreenHeight() / 2 + 14, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 18, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 18, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 22, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 22, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 26, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 26, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 29, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 29, radiusForBalls);
		AddNewCircleCollider(0, 0, 3.0f); //Player Collider


		
		
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

		//path.normOfSpline = .0f;
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
		float angle = atan2(-g1.y, g1.x);//arctan value of angle (in radian)
		//cout << angle*180/3.14159265f << endl;
		//FillCircle(p1.x, p1.y, 3, olc::WHITE);
		//DrawLine((3.0f * sin(angle) + p1.x), (3.0f * cos(angle) + p1.y), (-3.0f * sin(angle) + p1.x), (-3.0f * cos(angle) + p1.y), olc::MAGENTA);

		//In order to visualize
		//DrawString(2, 2, to_string(fMarker),olc::WHITE, 1);
		//DrawString(2, 8, to_string(offSet),olc::WHITE, 1);


		auto DoCirclesOverlap = [](float x1, float y1, float r1, float x2, float y2, float r2)
		{
			return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) <= (r1 + r2) * (r1 + r2);
		};

		auto IsPointInCircle = [](float x1, float y1, float r1, float px, float py)
		{
			return fabs((x1 - px) * (x1 - px) + (y1 - py) * (y1 - py)) < (r1 * r1);
		};

		if (/*GetMouse(0).bPressed ||*/ GetMouse(1).bPressed)
		{
			pSelectedBall = nullptr;
			for (auto& ball : vectorOfAllColliders)
			{
				if (IsPointInCircle(ball.px, ball.py, ball.radius, GetMouseX(), GetMouseY()))
				{
					pSelectedBall = &ball;
					break;
				}
			}
		}
		if (GetMouse(0).bHeld)
		{
			if (pSelectedBall != nullptr)
			{
				pSelectedBall->px = GetMouseX();
				pSelectedBall->py = GetMouseY();
			}
		}

		if (GetMouse(0).bReleased)
		{
			pSelectedBall = nullptr;
		}
		
		if (GetMouse(1).bReleased)
		{
			if (pSelectedBall != nullptr)
			{
				// Apply velocity
				pSelectedBall->vx = 5.0f * ((pSelectedBall->px) - (float)GetMouseX());
				pSelectedBall->vy = 5.0f * ((pSelectedBall->py) - (float)GetMouseY());
				
			}

			pSelectedBall = nullptr;
		}

		vector<pair<CircleCollider*, CircleCollider*>> vectorOfCollidingTwo;
		// Update Ball Positions
		for (auto& circleCol : vectorOfAllColliders)
		{
			// Add Drag to emulate rolling friction
			circleCol.ax = 0.5f *-circleCol.vx;
			circleCol.ay = 0.5f *-circleCol.vy;

			// Update ball physics
			circleCol.vx += circleCol.ax * fElapsedTime;
			circleCol.vy += circleCol.ay * fElapsedTime;
			circleCol.px += circleCol.vx * fElapsedTime;
			circleCol.py += circleCol.vy * fElapsedTime;

			// wrap the balls around screen
			if (circleCol.px < 0) circleCol.px += (float)ScreenWidth();
			if (circleCol.px >= ScreenWidth()) circleCol.px -= (float)ScreenWidth();
			if (circleCol.py < 0) circleCol.py += (float)ScreenHeight();
			if (circleCol.py >= ScreenHeight()) circleCol.py -= (float)ScreenHeight();

			// Clamp velocity near zero
			if (circleCol.vx * circleCol.vx + circleCol.vy * circleCol.vy < 0.01f)
			{
				circleCol.vx = 0;
				circleCol.vy = 0;
			}
		}
		// Static collisions
		for (auto& ball : vectorOfAllColliders)
		{
			for (auto& target : vectorOfAllColliders)
			{
				if (ball.id != target.id)
				{
					if (DoCirclesOverlap(ball.px, ball.py, ball.radius, target.px, target.py, target.radius))
					{
						// Collision has occured
						vectorOfCollidingTwo.push_back({ &ball, &target });

						// Distance between ball centers
						float fDistance = sqrtf((ball.px - target.px) * (ball.px - target.px) + (ball.py - target.py) * (ball.py - target.py));

						// Calculate displacement required
						float fOverlap = 0.5f * (fDistance - ball.radius - target.radius);

						// Displace Current Ball away from collision
						ball.px -= fOverlap * (ball.px - target.px) / fDistance;
						ball.py -= fOverlap * (ball.py - target.py) / fDistance;

						// Displace Target Ball away from collision
						target.px += fOverlap * (ball.px - target.px) / fDistance;
						target.py += fOverlap * (ball.py - target.py) / fDistance;
					}
				}
			}
		}

		//dynamic collisions
		for (auto c : vectorOfCollidingTwo)
		{
			CircleCollider* b1 = c.first;
			CircleCollider* b2 = c.second;

			// Distance between balls
			float fDistance = sqrtf((b1->px - b2->px) * (b1->px - b2->px) + (b1->py - b2->py) * (b1->py - b2->py));

			// Normal
			float nx = (b2->px - b1->px) / fDistance;
			float ny = (b2->py - b1->py) / fDistance;

			// Tangent
			float tx = -ny;
			float ty = nx;

			// Dot Product Tangent
			float dpTan1 = b1->vx * tx + b1->vy * ty;
			float dpTan2 = b2->vx * tx + b2->vy * ty;

			// Dot Product Normal
			float dpNorm1 = b1->vx * nx + b1->vy * ny;
			float dpNorm2 = b2->vx * nx + b2->vy * ny;

			// Conservation of momentum in 1D
			float m1 = (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2) / (b1->mass + b2->mass);
			float m2 = (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1) / (b1->mass + b2->mass);

			// Update ball velocities
			b1->vx = tx * dpTan1 + nx * m1;
			b1->vy = ty * dpTan1 + ny * m1;
			b2->vx = tx * dpTan2 + nx * m2;
			b2->vy = ty * dpTan2 + ny * m2;

		}

		// Draw Balls
		for (auto circleCol : vectorOfAllColliders)
		{
			DrawWireFrameModel(defaultCircleCollider, circleCol.px, circleCol.py, atan2f(circleCol.vy, circleCol.vx), circleCol.radius, olc::WHITE);
		}

		float characterXPosition = vectorOfAllColliders.back().px = p1.x;
		float characterYPosition = vectorOfAllColliders.back().py = p1.y;
		vectorOfAllColliders.back().vx = 1000.0f * fElapsedTime;
		vectorOfAllColliders.back().vy = 1000.0f * fElapsedTime;

		
		//DrawWireFrameModel(defaultCircleCollider, vectorOfAllColliders.back().px, vectorOfAllColliders.back().py, atan2f(g1.y, g1.x), 4, olc::BLANK);
		FillCircle(p1.x, p1.y, 4, olc::WHITE);
		DrawLine((3.0f * sin(angle) + p1.x), (3.0f * cos(angle) + p1.y), (-3.0f * sin(angle) + p1.x), (-3.0f * cos(angle) + p1.y), olc::MAGENTA);

		// Draw static collisions
		for (auto c : vectorOfCollidingTwo)
		{
			DrawLine(c.first->px, c.first->py, c.second->px, c.second->py, olc::RED);
		}

		// Draw Cue
		if (pSelectedBall != nullptr)
			DrawLine(pSelectedBall->px, pSelectedBall->py, GetMouseX(), GetMouseY(), olc::BLUE);
		return true;
	}
};

int main()
{
	Geo_Destruct application;
	application.Construct(120, 100, 8, 8, FULLSCREEN, VSYNC);
	application.Start();
	return 0;
}