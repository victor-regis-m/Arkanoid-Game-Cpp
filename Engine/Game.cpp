/****************************************************************************************** 
 *	Chili DirectX Framework Version 16.07.20											  *	
 *	Game.cpp																			  *
 *	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/
#include "MainWindow.h"
#include "Game.h"

Game::Game(MainWindow& wnd)
	:
	wnd(wnd),
	gfx(wnd),
	frameTimer(),
	ball(Vec2(400,515), Vec2(-200, -150)),
	pad(Vec2(400,530)),
	wall(RectF(150,650,10,590), 10 ,Color(200,100,140)),
	soundPad(L"Sounds\\arkpad.wav"),
	soundBrick(L"Sounds\\arkbrick.wav"),
	powerup(false, Vec2(400,300)),
	powerupWall(RectF(150, 650, 10, 555), 10, Color(200, 100, 140)),
	leftShot(Vec2(0,0), false),
	rightShot(Vec2(0,0), false)
{
	for (int j = 0; j < bricksVertical; j++)
	{
		Vec2 startPos = Vec2(160, 30);
		for (int i = 0; i < bricksHorizontal; i++)
		{
			Color c = colors[j%5];
			Vec2 brickPos = Vec2(brickWidth * (i), brickHeight * (j)) + startPos;
			bricks[i][j] = Brick(RectF(brickPos, brickWidth, brickHeight), c);
		}
	}
	walls = wall.GetInnerBounds();
	powerupWalls = powerupWall.GetInnerBounds();
}

void Game::Go()
{
	gfx.BeginFrame();	
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void Game::UpdateModel()
{
	float dt = frameTimer.deltaTime();
	pad.Move(wnd, dt);
	LimitPaddleToScreen();
	ball.Move(dt, pad);
	if (wnd.kbd.KeyIsPressed(VK_SPACE))
	{
		if(!ball.GetThownState())
			StartGame();
		if (pad.isWeaponActive && !leftShot.isActive && !rightShot.isActive)
		{
			leftShot = Shot(pad.GetLeftCannonPos(), true);
			rightShot = Shot(pad.GetRightCannonPos(), true);
			ammoCounter--;
		}
	}
	CheckAmmo();
	ShotMovement(dt);
	if(ball.DetectWallCollision(walls, dt))
		soundPad.Play();
	if(pad.isWallActive)
		if (ball.DetectWallCollision(powerupWalls, dt))
		{
			soundPad.Play();
			powerupWallLives--;
			if (powerupWallLives < 0)
				pad.isWallActive = false;
		}
	CheckBrickDestruction(dt);
	if(ball.DetectPadCollision(pad))
		soundPad.Play();
	CheckForDeath();
	CheckPowerupPosition();
	if (powerup.IsActive())
	{
		powerup.Move(dt);
		if (pad.PickUpPowerUp(powerup))
			CheckPowerupType();
	}
}

void Game::ComposeFrame()
{
	ball.Draw(gfx);
	wall.Draw(gfx);
	for (int i = 0; i < bricksHorizontal; i++)
		for (int j = 0; j < bricksVertical; j++)
			bricks[i][j].Draw(gfx);
	pad.Draw(gfx);
	ShowLivesLeft();
	powerup.Draw(gfx);
	if (pad.isWallActive)
		powerupWall.Draw(gfx);
	if (leftShot.isActive)
		leftShot.Draw(gfx, walls.top);
	if (rightShot.isActive)
		rightShot.Draw(gfx, walls.top);
}

void Game::LimitPaddleToScreen()
{
	float padLeftEdge = pad.GetLeftEdgePos();
	float padRightEdge = pad.GetRightEdgePos();
	if (padLeftEdge <= walls.left)
		pad.AdjustPadPosition(walls.left - padLeftEdge);
	if (padRightEdge >= walls.right)
		pad.AdjustPadPosition(walls.right - padRightEdge);

}

void Game::StartGame()
{
	Vec2 direction = (Vec2(wnd.mouse.GetPosX(), wnd.mouse.GetPosY()) - ball.GetPosition()).GetNormalized();
	ball.SetVelocity(direction * ballSpeed);
	ball.ThrowBall();
}

void Game::ShowLivesLeft()
{
	for (int i = 0; i < livesCounter; i++)
		SpriteCodex::DrawBall(Vec2(Graphics::ScreenWidth - 40 - 30 * i, 20), gfx);
}

void Game::CheckForDeath()
{
	if (ball.GetPosition().y > Graphics::ScreenHeight - 35)
	{
		ball.Copy(Ball{ Vec2(pad.PaddlePos().x, 515), Vec2(0,0) });
		livesCounter--;
	}
}

void Game::CheckPowerupPosition()
{
	if (powerup.GetPosition().y > Graphics::ScreenHeight - 30)
		powerup.Deactivate();
}

void Game::CheckPowerupType()
{
	int type = powerup.GetType();
	if (type == 0)
	{
		pad.isWallActive = true;
		powerupWallLives = 3;
	}
	else if (type == 1)
		pad.isTripleBallActive = true;
	else if (type == 2)
	{
		pad.isWeaponActive = true;
		ammoCounter = 10;
	}
}

void Game::ShotMovement(float dt)
{
	if (leftShot.isActive)
		leftShot.Move(dt, walls.top);
	if (rightShot.isActive)
		rightShot.Move(dt, walls.top);
}

void Game::CheckAmmo()
{
	if (ammoCounter <= 0)
		pad.isWeaponActive = false;

}

void Game::CheckBrickDestruction(float dt)
{
	for (int i = 0; i < bricksHorizontal; i++)
		for (int j = 0; j < bricksVertical; j++)
		{
			if (ball.DetectBrickCollision(bricks[i][j], dt))
			{
				soundBrick.Play();
				if (Powerups::GeneratePowerUp() && !powerup.IsActive())
					powerup = Powerups(true, bricks[i][j].GetPosition());
			}
			if (rightShot.isActive)
				if (bricks[i][j].isInsideBrick(rightShot.position))
				{
					bricks[i][j].DestroyBrick();
					rightShot.isActive = false;
				}
			if (leftShot.isActive)
				if (bricks[i][j].isInsideBrick(leftShot.position))
				{
					bricks[i][j].DestroyBrick();
					leftShot.isActive = false;
				}
		}
}

