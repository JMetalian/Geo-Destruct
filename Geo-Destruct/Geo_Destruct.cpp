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
	float speedOfAgent = 0.0f;

	/*----------------------------------------*/
	float fSliderValue = 50.0f;//Minimum 50
	void IncrementSlider() 
	{
		fSliderValue += 1.0f;
		if (fSliderValue > 100.0f) 
		{
			fSliderValue = 100.0f;
		}
	}

	void DecrementSlider() 
	{
		fSliderValue -= 1.0f;
		if (fSliderValue < 50.0f)
		{
			fSliderValue = 50.0f;
		}
	}
	float GetSliderValue() {
		return fSliderValue;
	}

	/*------------------------------------------*/
	vector<pair<float, float>> defaultCircleCollider;
	vector<CircleCollider> vectorOfAllColliders;
	CircleCollider* pSelecterCircle = nullptr;
	
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
		{
			defaultCircleCollider.push_back({ cosf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) , sinf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) });
		}

		float radiusForBalls = 3.0f;
		AddNewCircleCollider(ScreenWidth() / 2, ScreenHeight() / 2 + 5, radiusForBalls);
		AddNewCircleCollider(ScreenWidth() / 2 + 6, ScreenHeight() / 2 + 5, radiusForBalls);
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
		DrawString(ScreenWidth()/2-50,ScreenHeight()/2-50,"GEO-DESTRUCT",olc::WHITE,1);

		// Input Handling
		//Speed Control For Character
		if (GetKey(olc::A).bHeld)
		{
			DecrementSlider();
		}
		//Speed Control For Character
		if (GetKey(olc::D).bHeld) 
		{
			IncrementSlider();
		}

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
		if (GetKey(olc::Key::LEFT).bHeld)
		{	//Control the selected point
			path.points[selectedPointInPath].x -= 30.0f * fElapsedTime;
		}

		if (GetKey(olc::Key::RIGHT).bHeld)
		{	//Control the selected point
			path.points[selectedPointInPath].x += 30.0f * fElapsedTime;
		}

		if (GetKey(olc::Key::UP).bHeld)
		{	//Control the selected point
			path.points[selectedPointInPath].y -= 30.0f * fElapsedTime;
		}
		if (GetKey(olc::Key::DOWN).bHeld)
		{	//Control the selected point
			path.points[selectedPointInPath].y += 30.0f * fElapsedTime;
		}

		// Draw Spline
		for (float t = 0; t < (float)(path.points.size()-1); t += 0.005f)
		{
			Points position = path.GetSplinePoint(t, ISLOOPED);
			Draw(position.x, position.y);
		} 

		// Draw Control Points
		for (int i = 0; i < path.points.size(); i++)
		{
			path.normOfSpline +=(path.points[i].distanceBetweenPoints = path.GetPortionLength(i, ISLOOPED));
			DrawCircle(path.points[i].x,path.points[i].y, 2, olc::WHITE);
		}

		// Display controllers
		DrawLine(path.points[selectedPointInPath].x - 1, path.points[selectedPointInPath].y - 1, path.points[selectedPointInPath].x + 2, path.points[selectedPointInPath].y + 2, olc::GREEN);
			
		// Gradient demonstrater
		float offSet = path.NormalOffSet(speedOfAgent);
		if (offSet >=(float)(path.points.size() - 1))
		{
			offSet = (float)path.points.size() - 1.0f;
			speedOfAgent-=fSliderValue *fElapsedTime;
		}
		Points p1 = path.GetSplinePoint(offSet, ISLOOPED);
		Points g1 = path.GetSplineGradient(offSet, ISLOOPED);
		float angle = atan2(-g1.y, g1.x);//arctan value of angle (in radian)
		/*----------------------------------------------------------------------------------------*/

		//Draw Slider
		//FillRect(50, 50, 100, 10, olc::WHITE);
		//FillRect(50, 50, 70, 1, olc::WHITE);
		//FillRect(110, 10, 20
		FillRect(ScreenWidth()/2+30, ScreenHeight()/2+40, 25,1);
		//Slider Button
		int positionOfButtonX = (int)((GetSliderValue() - 50.0f)*.5f); //TODO
		FillRect(ScreenWidth() / 2 + 30 + positionOfButtonX, ScreenHeight() / 2 + 39, 1, 3, olc::RED);

		/*-------------------------------------------------------------------------------------------*/
		auto AreCirclesOverlappingWithEachOther = [](float x1, float y1, float r1, float x2, float y2, float r2)
		{
			return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) <= (r1 + r2) * (r1 + r2);
		};
		//Follow the Magenta-lined agent.
		float characterXPosition = vectorOfAllColliders.back().px = p1.x;
		float characterYPosition = vectorOfAllColliders.back().py = p1.y;

		//Move the character on the spline by pressing "W".
		if(GetKey(olc::Key::W).bPressed)
		{
			pSelecterCircle = nullptr;
			for (auto& circle : vectorOfAllColliders)
			{	//Set the collider to "Magenta-lined" agent.
				if (circle.px==p1.x || circle.py==p1.y)
				{
					pSelecterCircle = &circle;
					break;
				}
			}
		}
		//Give speed to "Magenta-lined agent"
		if (GetKey(olc::Key::W).bHeld)
		{
			speedOfAgent += /*50.0f*/fSliderValue * fElapsedTime;
		}
		if(GetKey(olc::Key::W).bReleased)
		{
			pSelecterCircle->vx = 0;
			pSelecterCircle->vy = 0;
			pSelecterCircle = nullptr;

		}
		if (speedOfAgent >= (float)path.normOfSpline)
		{
			speedOfAgent -= (float)path.normOfSpline;
		}
		if (speedOfAgent < 0.0f)
		{
			speedOfAgent += (float)path.normOfSpline;
		}
		vector<pair<CircleCollider*, CircleCollider*>> vectorOfCollidingTwo;
		// Update Circle Positions
		for (auto& circleCol : vectorOfAllColliders)
		{
			//Drag for slowing down
			circleCol.ax = 0.5f *-circleCol.vx;
			circleCol.ay = 0.5f *-circleCol.vy;

			//Circle velocity, position update
			circleCol.vx += circleCol.ax * fElapsedTime;
			circleCol.vy += circleCol.ay * fElapsedTime;
			circleCol.px += circleCol.vx * fElapsedTime;
			circleCol.py += circleCol.vy * fElapsedTime;

			//Circles may come up from other sides of the screen
			if (circleCol.px < 0) 
			{
				circleCol.px += (float)ScreenWidth();
			} 
			if (circleCol.px >= ScreenWidth()) 
			{
				circleCol.px -= (float)ScreenWidth();
			} 
			if (circleCol.py < 0)
			{
				circleCol.py += (float)ScreenHeight();
			}
			if (circleCol.py >= ScreenHeight())
			{
				circleCol.py -= (float)ScreenHeight();
			}

			//Set velocity of the object if it is too slow. 
			if (circleCol.vx * circleCol.vx + circleCol.vy * circleCol.vy < 0.01f)
			{
				circleCol.vx = 0;
				circleCol.vy = 0;
			}
		}
		// Static collisions
		for (auto& circle : vectorOfAllColliders)
		{
			for (auto& secondCircle : vectorOfAllColliders)
			{
				if (circle.id != secondCircle.id)
				{
					if (AreCirclesOverlappingWithEachOther(circle.px, circle.py, circle.radius, secondCircle.px, secondCircle.py, secondCircle.radius))
					{
						// Collision has occured
						vectorOfCollidingTwo.push_back({&circle, &secondCircle});

						//Give velocity to collider
						if (pSelecterCircle != nullptr)
						{
							// Simulate the collider's velocity which is behind the agent. Rotate collider according to normal vector of agent.
							pSelecterCircle->vx = speedOfAgent * cosf(angle);
							pSelecterCircle->vy = speedOfAgent * -sinf(angle);
						}
						// Distance between circle centers
						float distanceDifferenceBetweenCircles = sqrtf((circle.px - secondCircle.px) * (circle.px - secondCircle.px) + (circle.py - secondCircle.py) * (circle.py - secondCircle.py));

						// Calculate displacement required
						float requiredDisplacement = 0.5f * (distanceDifferenceBetweenCircles - circle.radius - secondCircle.radius);

						// Displace first circle away from collision
						circle.px -= requiredDisplacement * (circle.px - secondCircle.px) / distanceDifferenceBetweenCircles;
						circle.py -= requiredDisplacement * (circle.py - secondCircle.py) / distanceDifferenceBetweenCircles;

						// Displace second circle away from collision
						secondCircle.px += requiredDisplacement * (circle.px - secondCircle.px) / distanceDifferenceBetweenCircles;
						secondCircle.py += requiredDisplacement * (circle.py - secondCircle.py) / distanceDifferenceBetweenCircles;
					}
				}
			}
		}

		//Dynamic collisions
		for (auto c : vectorOfCollidingTwo)
		{
			CircleCollider* firstCircle = c.first;
			CircleCollider* secondCircle = c.second;

			// Distance between circles
			float distanceDifferenceBetweenCircles = sqrtf((firstCircle->px - secondCircle->px) * (firstCircle->px - secondCircle->px) + (firstCircle->py - secondCircle->py) * (firstCircle->py - secondCircle->py));

			// Normal. One unit vector
			float unitNormalX = (secondCircle->px - firstCircle->px) / distanceDifferenceBetweenCircles;
			float unitNormalY = (secondCircle->py - firstCircle->py) / distanceDifferenceBetweenCircles;

			// Tangent. One tangent vector
			float unitTangentX = -unitNormalY;
			float unitTangentY = unitNormalX;

			// Dot Product Tangent. Two scaler tangent value. One for first, one for second.
			float scalerVelocityOfFirstCircleInTangentialDirection = firstCircle->vx * unitTangentX + firstCircle->vy * unitTangentY;
			float scalerVelocityOfSecondCircleInTangentialDirection = secondCircle->vx * unitTangentX + secondCircle->vy * unitTangentY;

			// Dot Product Normal. Two scaler normal value. One for first, one for second.
			float scaleVelocityofFirstCircleInNormalDirection = firstCircle->vx * unitNormalX + firstCircle->vy * unitNormalY;
			float scaleVelocityofSecondCircleInNormalDirection = secondCircle->vx * unitNormalX + secondCircle->vy * unitNormalY;

			// Conservation of momentum formula. Normal velocities of circles. One for first after collision and one for second after collision.
			float afterCollisionNormalVelocityVectorOfFirstCircle = (scaleVelocityofFirstCircleInNormalDirection * (firstCircle->mass - secondCircle->mass) + 2.0f * secondCircle->mass * scaleVelocityofSecondCircleInNormalDirection) / (firstCircle->mass + secondCircle->mass);
			float afterCollisionNormalVelocityVectorOfSecondCircle = (scaleVelocityofSecondCircleInNormalDirection * (secondCircle->mass - firstCircle->mass) + 2.0f * firstCircle->mass * scaleVelocityofFirstCircleInNormalDirection) / (firstCircle->mass + secondCircle->mass);

			// Update circle velocities
			firstCircle->vx = (unitTangentX * scalerVelocityOfFirstCircleInTangentialDirection) + (unitNormalX * afterCollisionNormalVelocityVectorOfFirstCircle);
			firstCircle->vy = (unitTangentY * scalerVelocityOfFirstCircleInTangentialDirection) + (unitNormalY * afterCollisionNormalVelocityVectorOfFirstCircle);
			secondCircle->vx = (unitTangentX * scalerVelocityOfSecondCircleInTangentialDirection) + (unitNormalX * afterCollisionNormalVelocityVectorOfSecondCircle);
			secondCircle->vy = (unitTangentY * scalerVelocityOfSecondCircleInTangentialDirection) + (unitNormalY * afterCollisionNormalVelocityVectorOfSecondCircle);

		}

		// Draw circles
		for (auto circleCol : vectorOfAllColliders)
		{
			DrawWireFrameModel(defaultCircleCollider, circleCol.px, circleCol.py, atan2f(circleCol.vy, circleCol.vx), circleCol.radius, olc::WHITE);
		}
		
		FillCircle(p1.x, p1.y, 4, olc::WHITE);
		DrawLine((4.0f * sin(angle) + p1.x), (4.0f * cos(angle) + p1.y), (-4.0f * sin(angle) + p1.x), (-4.0f * cos(angle) + p1.y), olc::MAGENTA);

		// Draw collisions
		for (auto c : vectorOfCollidingTwo)
		{
			DrawLine(c.first->px, c.first->py, c.second->px, c.second->py, olc::RED);
		}
		
		//Draw the normal vector of agent.
		//DrawLine(10.0f * cosf(angle) + p1.x, 10.0f *-sinf(angle) + p1.y,-10.0f * cosf(angle) + p1.x, -10.0f * -sinf(angle) + p1.y,olc::BLUE);


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