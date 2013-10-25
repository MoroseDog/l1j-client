#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "sdl_master.h"
#include "client.h"
#include "resources/pack.h"

sdl_master::sdl_master(Uint32 flags)
{
	game_client = new SDL_Thread*[4];
	clients = new sdl_user*[4];
	for (int i = 0; i < 4; i++)
	{
		game_client[i] = (SDL_Thread*)0;
		clients[i] = (sdl_user*)0;
	}
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
	}
	else
	{
		display = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, flags);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		if (display == NULL) 
		{
			fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		}
		else
		{
			SDL_WM_SetCaption("Lineage", "Lineage");
			ready = 1;
		}
	}
}

void sdl_master::create_client()
{	
	if (game_client[0] == 0)
	{
		clients[0] = new sdl_user;
		game_client[0] = SDL_CreateThread(run_client, (void*)clients[0]);
		clients[0]->wait_ready();
	}
}


void sdl_master::process_events()
{
	bool done = false;
	
	SDL_MouseMotionEvent mouse_position;
	mouse_position.state = 0;
	mouse_position.x = 0;
	mouse_position.y = 0;
	
	SDL_Event event;
	while(!done)
	{
		draw();
		while(SDL_PollEvent(&event))
		{ //Poll events
			switch(event.type)
			{ //Check event type
				case SDL_MOUSEMOTION:
					mouse_move(&mouse_position, &event.motion);
					mouse_position = event.motion;
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					mouse_click(&event.button);
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					key_press(&event.key);
					break;
				case SDL_QUIT:
					quit_request();
					break;
				case SDL_VIDEORESIZE: //User resized window
					display = SDL_SetVideoMode(event.resize.w, event.resize.h, 16,
						SDL_HWSURFACE | SDL_DOUBLEBUF); // Create new window
					break;
			}
		}
		done = check_users(done);
	}
//	int status;
//	if (game_client[0] != 0)
//		SDL_WaitThread(game_client[0], &status);
}

bool sdl_master::check_users(bool last)
{
	if (clients[0] != 0)
	{
		if (clients[0]->done)
		{
			printf("Stop sdl_user\n");
			int bla;
			SDL_WaitThread(game_client[0], &bla);
			delete clients[0];
			clients[0] = 0;
			game_client[0] = (SDL_Thread*)0;
			last = true;
		}
	}
	return last;
}

void sdl_master::draw()
{
	SDL_Rect rect;
	if (clients[0] != 0)
	{
		clients[0]->draw();
		rect.x = 0;
		rect.y = 0;
		rect.w = clients[0]->display->w;
		rect.h = clients[0]->display->h;
		SDL_BlitSurface(clients[0]->display, &rect, display, &rect);
		SDL_Flip(display);
	}
	
}

bool sdl_master::quit_request()
{
	bool ok_to_quit = true;
	if (clients[0] != 0)
	{
		if (clients[0]->quit_request() == false)
		{
			ok_to_quit = false;
		}
	}
	return ok_to_quit;
}

void sdl_master::mouse_move(SDL_MouseMotionEvent *old, SDL_MouseMotionEvent *fresh)
{
	if (clients[0] != 0)
	{
		int old_num = get_client(old->x, old->y);
		if (old_num != get_client(fresh->x, fresh->y))
		{	//pointer tried to move from one client to another
			if (clients[old_num]->mouse_leave())
			{
				printf("Prevent client %d from losing the mouse\n", old_num);
				*fresh = *old;
			}
		}
		int new_num = get_client(fresh->x, fresh->y);
		//don't do an else here, the fresh mouse position may have changed
		if (old_num == get_client(fresh->x, fresh->y))
		{	//mouse moved within a single client
			clients[old_num]->mouse_move(old, fresh);
		}
		else
		{	//mouse was allowed to leave the old client
			clients[old_num]->mouse_from(old);
			clients[new_num]->mouse_to(fresh);
		}
	}
}

void sdl_master::mouse_click(SDL_MouseButtonEvent *here)
{
	if (clients[0] != 0)
	{
		//TODO: check for double-click by measuring the time since the last click
		int num = get_client(here->x, here->y);
		clients[num]->mouse_click(here);
	}
}

void sdl_master::key_press(SDL_KeyboardEvent *button)
{
	if (clients[0] != 0)
	{
		//TODO: determine which client gets keyboard input
		clients[0]->key_press(button);
	}
}

int sdl_master::get_client(int x, int y)
{	//determine which client owns the x/y coordinate specified
	return 0;
}

sdl_master::~sdl_master()
{
	delete [] game_client;
	delete [] clients;
	SDL_Quit();
}
