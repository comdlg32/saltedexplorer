#ifndef FAVORITES_INCLUDED
#define FAVORITES_INCLUDED

#include <list>
#include <boost/variant.hpp>

class Favorite;
class FavoriteFolder;

class FavoriteFolder
{
public:

	static FavoriteFolder	Create(const std::wstring &strName);
	static FavoriteFolder	*CreateNew(const std::wstring &strName);
	static FavoriteFolder	UnserializeFromRegistry(const std::wstring &strKey);
	~FavoriteFolder();

	void			SerializeToRegistry(const std::wstring &strKey);

	GUID			GetGUID() const;

	std::wstring	GetName() const;

	void			SetName(const std::wstring &strName);

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

	/* Returns true if this folder has *at least*
	one child folder. */
	bool			HasChildFolder() const;

	std::list<boost::variant<FavoriteFolder,Favorite>>::iterator	begin();
	std::list<boost::variant<FavoriteFolder,Favorite>>::iterator	end();

	void			InsertFavorite(const Favorite &bm);
	void			InsertFavoriteFolder(const FavoriteFolder &bf);
	void			InsertFavorite(const Favorite &bm,std::size_t Position);
	void			InsertFavoriteFolder(const FavoriteFolder &bf,std::size_t Position);

	void			RemoveFavorite();
	void			RemoveFavoriteFolder();

private:

	enum InitializationType_t
	{
		INITIALIZATION_TYPE_NORMAL,
		INITIALIZATION_TYPE_REGISTRY
	};

	FavoriteFolder(const std::wstring &str,InitializationType_t InitializationType);

	void			Initialize(const std::wstring &strName);
	void			InitializeFromRegistry(const std::wstring &strKey);

	GUID			m_guid;

	std::wstring	m_strName;

	/* Keeps track of the number of child
	folders that are added. Used purely as
	an optimization for the HasChildFolder()
	method above. */
	int				m_nChildFolders;

	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;

	/* List of child folders and FAVORITES. Note that
	the ordering within this list defines the ordering
	between child items (i.e. there is no explicit
	ordering). */
	std::list<boost::variant<FavoriteFolder,Favorite>>	m_ChildList;
};

class Favorite
{
public:

	Favorite(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription);
	~Favorite();

	GUID			GetGUID() const;

	std::wstring	GetName() const;
	std::wstring	GetLocation() const;
	std::wstring	GetDescription() const;

	void			SetName(const std::wstring &strName);
	void			SetLocation(const std::wstring &strLocation);
	void			SetDescription(const std::wstring &strDescription);

	int				GetVisitCount() const;
	FILETIME		GetDateLastVisited() const;

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

private:

	GUID			m_guid;

	std::wstring	m_strName;
	std::wstring	m_strLocation;
	std::wstring	m_strDescription;

	int				m_iVisitCount;
	FILETIME		m_ftLastVisited;

	FILETIME		m_ftCreated;
	FILETIME		m_ftModified;
};


#endif