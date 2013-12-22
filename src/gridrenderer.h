#ifndef GRIDRENDERER_H
#define GRIDRENDERER_H

#include "grid.h"
#include "SDL/SDL.h"
#include "tilerenderer.h"

class GridRenderer
{
private:
  SDL_Surface* m_dest;
  TileRenderer* m_tileRenderer;

public:
  GridRenderer(SDL_Surface* dest);

  GridRenderer(SDL_Surface* dest,
	       TileRenderer* tr);

  ~GridRenderer();
  
  template <class T>
  void RenderGrid(Grid<T>* grid)
  {
    Point<int> current;
    for(int x = 0; x < grid->GetWidth(); x++)
    {
      current.SetX(x);
      for(int y = 0; y < grid->GetHeight(); y++)
      {
	current.SetY(y);
	
	m_tileRenderer->RenderTile(grid->GetTile(x, y),
				   &current);
      }
    }
  }
  
};

#endif /*GRIDRENDERER_H*/
