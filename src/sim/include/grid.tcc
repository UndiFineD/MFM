                           /* -*- C++ -*- */
#include "Dirs.h"
#include "grid.h"

namespace MFM {

  template <class T,u32 R,u32 W, u32 H>
  Grid<T,R,W,H>::Grid() : m_seed(0), m_width(W), m_height(H)
  {
  }

  template <class T,u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::Reinit() {

    /* Reseed grid PRNG and push seeds to the tile PRNGs */
    ReinitSeed();

    /* Reinit all the tiles */

    /* Set the neighbors flags of each tile. This lets the tiles know */
    /* if any of its caches are dead and should not be written to.    */
    u8 neighbors;
    for(u32 x = 0; x < m_width; x++)
    {
      for(u32 y = 0; y < m_height; y++)
      {
	Tile<T,R>& ctile = GetTile(x, y);

        ctile.Reinit();

	neighbors = 0;
	if(x > 0)
        {
	  ctile.Connect(GetTile(x - 1, y), Dirs::WEST);
	  neighbors |= (1<<Dirs::WEST);
	}
	if(y > 0)
	{
	  ctile.Connect(GetTile(x, y - 1), Dirs::NORTH);
	  neighbors |= (1<<Dirs::NORTH);
	}
	if(x < m_width - 1)
        {
	  ctile.Connect(GetTile(x + 1, y), Dirs::EAST);
	  neighbors |= (1<<Dirs::EAST);
	}
	if(y < m_height - 1)
        {
	  ctile.Connect(GetTile(x, y + 1), Dirs::SOUTH);
	  neighbors |= (1<<Dirs::SOUTH);
	}
	if((neighbors & (1<<Dirs::SOUTH)) &&
	   (neighbors & (1<<Dirs::WEST)))
        {
	  ctile.Connect(GetTile(x - 1, y + 1), Dirs::SOUTHWEST);
	}
	if((neighbors & (1<<Dirs::NORTH)) &&
	   (neighbors & (1<<Dirs::WEST)))
        {
	  ctile.Connect(GetTile(x - 1, y - 1), Dirs::NORTHWEST);
	}
	if((neighbors & (1<<Dirs::SOUTH)) &&
	   (neighbors & (1<<Dirs::EAST)))
        {
	  ctile.Connect(GetTile(x + 1, y + 1), Dirs::SOUTHEAST);
	}
	if((neighbors & (1<<Dirs::NORTH)) &&
	   (neighbors & (1<<Dirs::EAST)))
        {
	  ctile.Connect(GetTile(x + 1, y - 1), Dirs::NORTHEAST);
	}
      }
    }
  }

  template <class T,u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::SetSeed(u32 seed)
  {
    m_seed = seed;
  }

  template <class T,u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::ReinitSeed()
  {
    if (m_seed==0)  // SetSeed must have been called by now!
      FAIL(ILLEGAL_STATE);

    m_random.SetSeed(m_seed);
    for(u32 i = 0; i < W; i++)
      for(u32 j = 0; j < H; j++)
        {
          m_tiles[i][j].GetRandom().SetSeed(m_random.Create());
        }
  }

  template <class T, u32 R,u32 W, u32 H>
  Grid<T,R,W,H>::~Grid()
  {
  }


  template <class T, u32 R,u32 W, u32 H>
  bool Grid<T,R,W,H>::IsLegalTileIndex(const SPoint & tileInGrid) const
  {
    if (tileInGrid.GetX() < 0 || tileInGrid.GetY() < 0)
      return false;
    if (tileInGrid.GetX() >= (s32) W || tileInGrid.GetY() >= (s32) H)
      return false;
    return true;
  }

  template <class T, u32 R,u32 W, u32 H>
  bool Grid<T,R,W,H>::MapGridToTile(const SPoint & siteInGrid, SPoint & tileInGrid, SPoint & siteInTile) const
  {
    SPoint myTile, mySite;
    if (!MapGridToUncachedTile(siteInGrid, myTile, mySite)) return false;
    tileInGrid = myTile;
    siteInTile = mySite+SPoint(R,R);      // adjust to full Tile indexing
    return true;
  }

  template <class T, u32 R,u32 W, u32 H>
  bool Grid<T,R,W,H>::MapGridToUncachedTile(const SPoint & siteInGrid, SPoint & tileInGrid, SPoint & siteInTile) const
  {
    if (siteInGrid.GetX() < 0 || siteInGrid.GetY() < 0)
      return false;

    SPoint t = siteInGrid/Tile<T,R>::OWNED_SIDE;

    if (!IsLegalTileIndex(t))
      return false;

    // Set up return values
    tileInGrid = t;
    siteInTile =
      siteInGrid % Tile<T,R>::OWNED_SIDE;  // get index into just 'owned' sites
    return true;
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::PlaceAtom(T& atom, const SPoint& siteInGrid)
  {
    SPoint tileInGrid, siteInTile;
    if (!MapGridToTile(siteInGrid, tileInGrid, siteInTile))
      FAIL(ILLEGAL_ARGUMENT);  // XXX Change to return bool?

    Tile<T,R> & owner = GetTile(tileInGrid);
    owner.PlaceAtom(atom, siteInTile);

    Dir startDir = owner.CacheAt(siteInTile);

    if ((s32) startDir < 0)       // Doesn't hit cache, we're done
      return;

    Dir stopDir = Dirs::CWDir(startDir);

    if (Dirs::IsCorner(startDir)) {
      startDir = Dirs::CCWDir(startDir);
      stopDir = Dirs::CWDir(stopDir);
    }

    for (Dir dir = startDir; dir != stopDir; dir = Dirs::CWDir(dir)) {
      SPoint tileOffset;
      Dirs::FillDir(tileOffset,dir);

      SPoint otherTileIndex = tileInGrid+tileOffset;

      if (!IsLegalTileIndex(otherTileIndex)) continue;  // edge of grid

      Tile<T,R> & other = GetTile(otherTileIndex);
      SPoint otherIndex = siteInTile + tileOffset * Tile<T,R>::OWNED_SIDE;

      other.PlaceAtom(atom,otherIndex);
      FAIL(INCOMPLETE_CODE);
    }

  }

  template <class T, u32 R,u32 W, u32 H>
  const T* Grid<T,R,W,H>::GetAtom(SPoint& loc)
  {
    SPoint tileInGrid, siteInTile;
    if (!MapGridToTile(loc, tileInGrid, siteInTile))
      FAIL(ILLEGAL_ARGUMENT);  // XXX Change to return bool?

    return GetTile(tileInGrid).GetAtom(siteInTile);
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::Pause()
  {
    for(u32 x = 0; x < W; x++)
    {
      for(u32 y = 0; y < H; y++)
      {
	GetTile(x, y).Pause();
      }
    }
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::Unpause()
  {
    for(u32 x = 0; x < W; x++)
    {
      for(u32 y = 0; y < H; y++)
      {
	GetTile(x, y).Start();
      }
    }
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::TriggerEvent()
  {
    /*Change to 0 if aiming a window at a certian tile.*/
#if 1
    SPoint windowTile(GetRandom(), m_width, m_height);
#else
    SPoint windowTile(0, 1);
#endif

    Tile<T,R>& execTile = m_tiles[windowTile.GetX()][windowTile.GetY()];

    //  execTile.Execute(ElementTable<T,R>::get());
    execTile.Execute();

    m_lastEventTile.Set(windowTile.GetX(), windowTile.GetY());
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::FillLastEventTile(SPoint& out)
  {
    out.Set(m_lastEventTile.GetX(),
            m_lastEventTile.GetY());
  }

  template <class T, u32 R, u32 W, u32 H>
  u64 Grid<T,R,W,H>::GetTotalEventsExecuted() const
  {
    u64 total = 0;
    for(u32 x = 0; x < W; x++)
    {
      for(u32 y = 0; y < H; y++)
      {
	total += m_tiles[x][y].GetEventsExecuted();
      }
    }
    return total;
  }

  template <class T, u32 R, u32 W, u32 H>
  void Grid<T,R,W,H>::WriteEPSImage(FILE* outstrm) const
  {
    u64 max = 0;
    const u32 swidth = GetWidthSites();
    const u32 sheight = GetHeightSites();

    for(u32 pass = 0; pass < 2; ++pass) {
      if (pass==1) 
        fprintf(outstrm,"P5\n # Max site events = %ld\n%d %d 255\n",max,swidth,sheight);
      for(u32 y = 0; y < sheight; y++) {
	for(u32 x = 0; x < swidth; x++) {
          SPoint siteInGrid(x,y), tileInGrid, siteInTile;
          if (!MapGridToUncachedTile(siteInGrid, tileInGrid, siteInTile))
            FAIL(ILLEGAL_STATE);
          u64 events = GetTile(tileInGrid).GetUncachedSiteEvents(siteInTile);
          if (pass==0)
            max = MAX(max, events); 
          else
            fputc((u8) (events*255/max), outstrm);
        }
      }
    }
  }

  template <class T, u32 R,u32 W, u32 H>
  u32 Grid<T,R,W,H>::GetAtomCount(ElementType atomType) const
  {
    u32 total = 0;
    for(u32 i = 0; i < W; i++)
      for(u32 j = 0; j < H; j++)
        total += m_tiles[i][j].GetAtomCount(atomType);

    return total;
  }

  template <class T, u32 R,u32 W, u32 H>
  void Grid<T,R,W,H>::Needed(const Element<T,R> & anElement)
  {
    for(u32 i = 0; i < W; i++)
      for(u32 j = 0; j < H; j++)
        m_tiles[i][j].RegisterElement(anElement);
  }
} /* namespace MFM */

