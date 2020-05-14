#pragma once

#include "../Ares.CRT.h"

#include "../Misc/Savegame.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <ArrayClasses.h>
#include <CCINIClass.h>

template <typename T> class Enumerable
{
	typedef std::vector<std::unique_ptr<T>> container_t;
public:
	static container_t Array;

	static int FindIndex(const char *Title)
	{
		auto result = std::find_if(Array.begin(), Array.end(), [Title](std::unique_ptr<T> &Item) {
			return !_strcmpi(Item->Name, Title);
		});
		if(result == Array.end()) {
			return -1;
		}
		return std::distance(Array.begin(), result);
	}

	static T* Find(const char *Title)
	{
		auto result = FindIndex(Title);
		return (result < 0) ? nullptr : Array[static_cast<size_t>(result)].get();
	}

	static T* FindOrAllocate(const char *Title)
	{
		if(T *find = Find(Title)) {
			return find;
		}
		Array.push_back(std::make_unique<T>(Title));
		return Array.back().get();
	}

	static void Clear()
	{
		Array.clear();
	}

	static void LoadFromINIList(CCINIClass *pINI)
	{
		const char *section = GetMainSection();
		int len = pINI->GetKeyCount(section);
		for(int i = 0; i < len; ++i) {
			const char *Key = pINI->GetKeyName(section, i);
			FindOrAllocate(Key);
		}
		for(size_t i = 0; i < Array.size(); ++i) {
			Array[i]->LoadFromINI(pINI);
		}
	}

	static bool LoadGlobals(AresStreamReader& Stm)
	{
		Clear();

		size_t Count = 0;
		if(!Stm.Load(Count)) {
			return false;
		}

		for(size_t i = 0; i < Count; ++i) {
			void* oldPtr = nullptr;
			decltype(Name) name;
			if(!Stm.Load(oldPtr) || !Stm.Load(name)) {
				return false;
			}

			auto newPtr = FindOrAllocate(name);
			AresSwizzle::Instance.RegisterChange(oldPtr, newPtr);

			newPtr->LoadFromStream(Stm);
		}

		return true;
	}

	static bool SaveGlobals(AresStreamWriter& Stm)
	{
		Stm.Save(Array.size());

		for(const auto& item : Array) {
			// write old pointer and name, then delegate
			Stm.Save(item.get());
			Stm.Save(item->Name);
			item->SaveToStream(Stm);
		}

		return true;
	}

	static const char * GetMainSection();

	Enumerable(const char* Title) {
		this->Name[0] = 0;

		if(Title) {
			AresCRT::strCopy(this->Name, Title);
		}
	}

	virtual ~Enumerable() = default;

	virtual void LoadFromINI(CCINIClass *pINI) {}

	virtual void LoadFromStream(AresStreamReader &Stm) = 0;

	virtual void SaveToStream(AresStreamWriter &Stm) = 0;

	char Name[32];
};