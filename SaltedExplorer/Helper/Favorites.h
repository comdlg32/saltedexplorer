#ifndef FAVORITES_INCLUDED
#define FAVORITES_INCLUDED

#include <list>
#include <boost/variant.hpp>

class CFavorite;
class CFavoriteFolder;

namespace NFavorite
{
	__interface IFavoriteItemNotification
	{
		virtual void	OnFavoriteItemModified(const GUID &guid);

		virtual void	OnFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite);
		virtual void	OnFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder);

		virtual void	OnFavoriteRemoved(const GUID &guid);
		virtual void	OnFavoriteFolderRemoved(const GUID &guid);
	};
}

class CFavoriteFolder
{
public:

	static CFavoriteFolder	Create(const std::wstring &strName);
	static CFavoriteFolder	*CreateNew(const std::wstring &strName);
	static CFavoriteFolder	UnserializeFromRegistry(const std::wstring &strKey);
	~CFavoriteFolder();

	void			SerializeToRegistry(const std::wstring &strKey);

	GUID			GetGUID() const;

	std::wstring	GetName() const;

	void			SetName(const std::wstring &strName);

	FILETIME		GetDateCreated() const;
	FILETIME		GetDateModified() const;

	/* Returns true if this folder has *at least*
	one child folder. */
	bool			HasChildFolder() const;

	std::list<boost::variant<CFavoriteFolder,CFavorite>>::iterator	begin();
	std::list<boost::variant<CFavoriteFolder,CFavorite>>::iterator	end();

	std::list<boost::variant<CFavoriteFolder,CFavorite>>::const_iterator	begin() const;
	std::list<boost::variant<CFavoriteFolder,CFavorite>>::const_iterator	end() const;

	void			InsertFavorite(const CFavorite &Favorite);
	void			InsertFavorite(const CFavorite &Favorite,std::size_t Position);
	void			InsertFavoriteFolder(const CFavoriteFolder &FavoriteFolder);
	void			InsertFavoriteFolder(const CFavoriteFolder &FavoriteFolder,std::size_t Position);

	void			RemoveFavorite();
	void			RemoveFavoriteFolder();

private:

	enum InitializationType_t
	{
		INITIALIZATION_TYPE_NORMAL,
		INITIALIZATION_TYPE_REGISTRY
	};

	CFavoriteFolder(const std::wstring &str,InitializationType_t InitializationType);

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
	std::list<boost::variant<CFavoriteFolder,CFavorite>>	m_ChildList;
};

class CFavorite
{
public:

	CFavorite(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription);
	~CFavorite();

	GUID			GetGUID() const;

	std::wstring	GetName() const;
	std::wstring	GetLocation() const;
	std::wstring	GetDescription() const;

	void			SetName(const std::wstring &strName);
	void			SetLocation(const std::wstring &strLocation);
	void			SetDescription(const std::wstring &strDescription);

	int				GetVisitCount() const;
	FILETIME		GetDateLastVisited() const;

	void			UpdateVisitCount();

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

class CFavoriteItemNotifier
{
public:

	~CFavoriteItemNotifier();

	static CFavoriteItemNotifier &GetInstance();

	void	AddObserver(NFavorite::IFavoriteItemNotification *pbin);
	void	RemoveObserver(NFavorite::IFavoriteItemNotification *pbin);

	void	NotifyObserversFavoriteItemModified(const GUID &guid);
	void	NotifyObserversFavoriteAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavorite &Favorite);
	void	NotifyObserversFavoriteFolderAdded(const CFavoriteFolder &ParentFavoriteFolder,const CFavoriteFolder &FavoriteFolder);
	void	NotifyObserversFavoriteRemoved(const GUID &guid);
	void	NotifyObserversFavoriteFolderRemoved(const GUID &guid);

private:

	enum NotificationType_t
	{
		NOTIFY_FAVORITE_ITEM_MODIFIED,
		NOTIFY_FAVORITE_ADDED,
		NOTIFY_FAVORITE_FOLDER_ADDED,
		NOTIFY_FAVORITE_REMOVED,
		NOTIFY_FAVORITE_FOLDER_REMOVED
	};

	CFavoriteItemNotifier();

	CFavoriteItemNotifier(const CFavoriteItemNotifier &);
	CFavoriteItemNotifier & operator=(const CFavoriteItemNotifier &);

	void	NotifyObservers(NotificationType_t NotificationType,const CFavoriteFolder *pParentFavoriteFolder,const CFavoriteFolder *pFavoriteFolder,const CFavorite *pFavorite,const GUID *pguid);

	std::list<NFavorite::IFavoriteItemNotification *>	m_listObservers;
};

#endif