// Inventory.cpp : Defines the entry point for the console application.
// Author : ravi rajamani
// This code implements the specification in Problem B Inventory Program
//

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <string>

using namespace std;

namespace InventoryItems
{
	const int MAX_NAME_LENGTH = 20;
	const int MAX_ELEMENTS = 100;


	struct Item
	{
	_TCHAR name[MAX_NAME_LENGTH+1];
	float buyAt;
	float sellAt;
	int numBought;
	int numSold;
	};

	class Inventory
	{
		Item  catalog[MAX_ELEMENTS];               // collection of Items stocked in the inventory
		map<basic_string<_TCHAR>, int> m; // sorted pairing of Item name and index into catalog
		int index;                        // next available index into catalog for new item
		
		float writeOff;                   // cumulative value of items written off as yet
		float lastProfit;                 // last known profit snapshot

	public:
		Inventory();
		~Inventory();
		void AddItem ( _TCHAR* name, int len, float buyAt, float sellAt );
		void DeleteItem ( _TCHAR* name, int len );
		void BuyItem ( _TCHAR* name, int len, int quantity );
		void SellItem ( _TCHAR* name, int len, int quantity );
		void Report ( );

	private:
		int GetItemIndex ( _TCHAR* name, int len );
		void SetItemIndex ( _TCHAR* name, int len, int value );
		Inventory( const Inventory& ) throw ( std::bad_exception );
		Inventory& operator= ( const Inventory&) throw ( std::bad_exception );
	};

	Inventory::Inventory ( )
	{
	  // ZeroMemory( catalog, 100*sizeof(catalog) );
		for ( int i = 0 ; i < MAX_ELEMENTS; i++ )
		{
			_tcscpy( catalog[i].name, _T(""));
			catalog[i].buyAt = 0;
			catalog[i].sellAt = 0;
			catalog[i].numBought = 0;
			catalog[i].numSold = 0;
		}

		index = 0;
		writeOff = 0;
		lastProfit = 0;
	}

	Inventory::~Inventory ( )
	{
		m.clear();
	}

	Inventory::Inventory( const Inventory& ) throw ( std::bad_exception )
	{
		// not allowed
		throw std::bad_exception();
	}

	Inventory& Inventory::operator= ( const Inventory& i) throw ( std::bad_exception )
	{
		// not implemented or allowed
		throw std::bad_exception();
		return *this;
	}

	// AddItem stores item details in catalog at next available slot
	//
	void Inventory::AddItem ( _TCHAR* name, int len, float buyAt, float sellAt )
	{
		if ( index < MAX_ELEMENTS )
		{
			_tcsncpy(catalog[index].name, name, MAX_NAME_LENGTH );
			catalog[index].buyAt = buyAt;
			catalog[index].sellAt = sellAt;
			SetItemIndex( name, len, index );
			index++;
		}
		else throw std::bad_exception();
	}

	// DeleteItem drops the sales price of an Item such that the cost is written off
	//
	void Inventory::DeleteItem ( _TCHAR* name, int len )
	{
		int i = GetItemIndex( name, len );
		catalog[i].sellAt = 0;
		writeOff += ( catalog[i].numBought - catalog[i].numSold ) * catalog[i].buyAt;
	}

	// BuyItem updates bookkeeping of the number of items added to existing Inventory
	//
	void Inventory::BuyItem ( _TCHAR* name, int len, int quantity )
	{
		int i = GetItemIndex ( name, len );
		catalog[i].numBought += quantity;
	}

	// Sell Item updates bookkeeping of the number of items removed from Inventory
	//
	void Inventory::SellItem ( _TCHAR* name, int len, int quantity )
	{
		int i = GetItemIndex ( name, len );
		catalog[i].numSold += quantity;
	}

	// Report publishes bottomline
	//
	void Inventory::Report ( )
	{
		FILE* fout = 0;
		errno_t err = _tfopen_s ( &fout, _TEXT("C:\\InventoryReport.txt"), _TEXT("a+") );

		if ( !fout || err != 0 ) 	
		{
			cout << "C:\\InventoryReport.txt could not be opened" << endl;
			return;
		}

		_ftprintf_s( fout, _T("                        INVENTORY REPORT\n"));
		_ftprintf_s( fout, _T("Item Name           Buy At\t\tSell At\t\tOn Hand\t\t  Value\n"));
		_ftprintf_s( fout, _T("---------           ------\t\t-------\t\t-------\t\t  -----\n"));

		float totalValue = 0;
		float totalSales = 0;
		float totalCostOfItemsSold = 0;

		typedef map<basic_string<_TCHAR>, int>::const_iterator CI;
		for ( CI e = m.begin(); e != m.end(); ++e )
		{
			int i = e->second;
			if ( catalog[i].buyAt != 0 && catalog[i].sellAt != 0 )
			{
				_ftprintf_s( fout, _T("%s"), catalog[i].name );
				int numSpace = MAX_NAME_LENGTH - _tcslen(catalog[i].name);
				if ( numSpace > 0 )
					for ( int j = 0; j < numSpace ; j++ )
						_ftprintf_s( fout, _T(" ") );

				_ftprintf_s( fout, _T("%5.2f\t\t"), catalog[i].buyAt );
				_ftprintf_s( fout, _T("%5.2f\t\t"), catalog[i].sellAt );
				int numOnHand = catalog[i].numBought - catalog[i].numSold;
				_ftprintf_s( fout, _T("%5d\t\t"), numOnHand );
				float value = numOnHand*catalog[i].buyAt;
				_ftprintf_s( fout, _T("%7.2f\t\t\n"), value );
				
				totalValue += value;
				totalSales += catalog[i].numSold*catalog[i].sellAt;
				totalCostOfItemsSold += catalog[i].numSold*catalog[i].buyAt;
			}
		}
		_ftprintf_s( fout, _T("------------------------------\n"));
		_ftprintf_s( fout, _T("Total Value of inventory  \t\t       \t\t       \t\t%7.2f\n"), totalValue);
		float profit = totalSales - totalCostOfItemsSold - writeOff;
		float diff = profit - lastProfit;
		lastProfit = profit;
		_ftprintf_s( fout, _T("Profit since last report  \t\t       \t\t       \t\t%7.2f\n"),  diff);

		fclose(fout);
	}

	// GetItemIndex retrieves the position of an Item in the catalog based on it's name
	//
	int Inventory::GetItemIndex( _TCHAR* name, int len )
	{
		basic_string<_TCHAR> s( name );
		return m[s];
	}

	// SetItemIndex sets the position for an Item in the catalog with every AddItem
	//
	void Inventory::SetItemIndex( _TCHAR* name, int len, int value )
	{
		basic_string<_TCHAR> s ( name );
		map<basic_string<_TCHAR>, int>::const_iterator CI;
		CI = m.find(s);
		if ( CI != m.end() ) 
			throw std::bad_exception();
		m[s] = value;
	}

	// Populates Sample Input for validating this program
	//
	void PopulateSampleInput ()
	{
		FILE* fout = 0;
		errno_t err = _tfopen_s( &fout,  _TEXT("C:\\SampleInput.txt"), _TEXT("w+") );
		if ( !fout || err != 0 )
		{
			cout << "C:\\SampleInput.txt cannot be created!" << endl;
			return;
		}
		_ftprintf_s( fout,	_T( "new Sunglasses01 0.50 3.79\n") \
							_T(	"new Towel01 1.47 6.98\n") \
							_T(	"new Sunglasses02 0.63 4.29\n") \
							_T(	"new Sunblock 1.00 4.98\n") \
							_T(	"buy Sunblock 100\n") \
							_T(	"sell Sunblock 2\n") \
							_T(	"buy Towel01 500\n") \
							_T(	"buy Sunglasses01 100\n") \
							_T(	"buy Sunglasses02 100\n") \
							_T(	"sell Towel01 1\n") \
							_T(	"sell Towel01 1\n") \
							_T(	"sell Sunblock 2\n") \
							_T(	"report\n") \
							_T(	"delete Sunglasses01\n") \
							_T(	"sell Sunblock 5\n") \
							_T(	"new Sunglasses03 .51 1.98\n") \
							_T(	"buy Sunglasses03 250\n") \
							_T(	"sell Towel01 5\n") \
							_T(	"sell Sunglasses03 4\n") \
							_T(	"sell Sunglasses02 10\n") \
							_T(	"report\n") \
							_T(	"*\n" ));
		fclose(fout);

	}
};
using namespace InventoryItems;

// main 
//
int _tmain(int argc, _TCHAR* argv[])
{
	

	int len = 100;
	_TCHAR buf[100] = {0};

	FILE* fin = 0;
	PopulateSampleInput();
	errno_t err = _tfopen_s( &fin,  _TEXT("C:\\SampleInput.txt"), _TEXT("r+") );
	if ( !fin || err != 0 )
	{
		cout << "C:\\SampleInput.txt doesn't exist!" << endl;
		return -1;
	}

	try
	{

		auto_ptr<Inventory> store( new Inventory );

		while ( !feof(fin) )
		{
			_ftscanf_s( fin, _T("%s\n"), buf, len );
			if ( _tcsnicmp( buf, _T("new"), _tcslen(_T("new"))) == 0 )
			{
				float buyAt = 0;
				float sellAt = 0;
				_TCHAR name[MAX_NAME_LENGTH+1] = {0};
				_ftscanf_s( fin, _T("%s"), name, MAX_NAME_LENGTH);
				_ftscanf_s( fin, _T("%f"), &buyAt, sizeof(int));
				_ftscanf_s( fin, _T("%f"), &sellAt, sizeof(int));
				store->AddItem ( name, _tcslen(name), buyAt, sellAt );
				continue;

			}
			if ( _tcsnicmp( buf, _T("delete"), _tcslen(_T("delete"))) == 0 )
			{
				_TCHAR name[MAX_NAME_LENGTH+1] = {0};
				_ftscanf_s( fin, _T("%s"), name, MAX_NAME_LENGTH );
				store->DeleteItem ( name, _tcslen(name) );
				continue;
			}
			if ( _tcsnicmp( buf, _T("buy"), _tcslen(_T("buy"))) == 0 )
			{
				int quantity = 0;
				_TCHAR name[MAX_NAME_LENGTH+1] = {0};
				_ftscanf_s( fin, _T("%s"), name, MAX_NAME_LENGTH );
				_ftscanf_s( fin, _T("%d"), &quantity );
				store->BuyItem( name, _tcslen(name), quantity );
				continue;
			}
			if ( _tcsnicmp( buf, _T("sell"), _tcslen(_T("sell"))) == 0 )
			{
				int quantity = 0;
				_TCHAR name[MAX_NAME_LENGTH+1] = {0};
				_ftscanf_s( fin, _T("%s"), name, MAX_NAME_LENGTH );
				_ftscanf_s( fin, _T("%d"), &quantity );
				store->SellItem( name, _tcslen(name), quantity );
				continue;
			}
			if ( _tcsnicmp( buf, _T("report"), _tcslen(_T("report"))) == 0 )
			{
				store->Report();
				continue;
			}

			if ( _tcsnicmp( buf, _T("*"), sizeof(_TCHAR) ) == 0 )
			{
				break;
			}

			throw std::bad_exception();
		}
	}
	catch ( ... )
	{
		cout << "program failed" << endl;
	}

	fclose(fin);
	return 0;
}

