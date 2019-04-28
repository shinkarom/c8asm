#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

stringstream ss(stringstream::binary|stringstream::out);

unordered_map<string, int> symtable;
unordered_map<string, int> regtable;
unordered_map<int, string> labelrefs;

int addr;

#include "tokenizer.hpp"
#include "emitter.hpp"

void process_one_db()
{
	int r=expect_numberthing();
	if(r>255){
		cout<<"Number too large."<<endl;
		exit(0);
	}else{
		ss.write((char*)&r,1);
		addr++;
	}
}

void process_db()
{
	expect_whitespace();
	process_one_db();
	while(optional_comma())
	{
		process_one_db();
	}
	cout<<"processed db"<<endl;	
}

void process_one_dw()
{
	int r = expect_numberthing();
	if(r>65535){
		cout<<"Number too large."<<endl;
		exit(0);
	}else{
		emit_oneword(r);
		addr+=2;
	}	
}

void process_dw()
{
	expect_whitespace();
	process_one_dw();
	while(optional_comma())
	{
		process_one_dw();		
	}
	cout<<"processed dw"<<endl;	
}

void process_ld()
{
	auto mn = peek_token();
	if(is_registerthing(mn))
	{
		auto x = expect_registerthing();
		expect_comma();
		
	}
	else if(mn=="dt")
	{
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x15);
	}
	else if(mn=="st")
	{
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x18);
	}
	else if(mn=="f")
	{
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x29);
	}
	else if(mn=="b")
	{
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x33);
	}
	else if(mn=="i")
	{
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x55);
	}
}

void process_mnemonic()
{
	auto mnemonic = get_token();
	if (!is_reserved(mnemonic)) return;	
	if(mnemonic=="db"){	
		process_db();
	} 
	else if(mnemonic=="dw"){
		process_dw();
	}
	else if (mnemonic=="cls"){
		emit_twobytes(0x00,0xE0);
		addr+=2;
	}
	else if (mnemonic=="ret"){
		emit_twobytes(0x00,0xEE);
		addr+=2;		
	}	
	else if (mnemonic=="ld")
	{
		expect_whitespace();
		process_ld();
		addr+=2;
	}
	cout<<"mnemonic "<<mnemonic<<endl;
	auto tk = get_token();
	if(tk!="")
	{
		cout<<"Extraneous token "<<tk<<" on the line."<<endl;
		exit(0);
	}		
}

int main(int argc, char *argv[])
{
	ifstream ifs("test.asm",ios::in);
	char templine[256];
	addr = 0x200;
	while(ifs.getline(templine,256))
	{
		string thisline(templine);
		thisline = thisline.substr(0,thisline.find(";",0));;
		if(thisline.size()>0)
		{
			init_tokenizer(thisline);
			auto tok1 = peek_token();
			if(is_reserved(tok1))			
			{ 
				process_mnemonic();
			}
			else 	
			{
			get_token();	
			if(auto tok2 = peek_token(); tok2=="equ")
			{		
				get_token();
				int i;
				auto tok3 = peek_token();
				if(is_number(tok3)){	
				i = get_number();
					symtable[tok1] = i;
				}else if(is_register(tok3)){
					i=get_register();
					regtable[tok1] = i;
				} else{
					cout<<"Wrong constant."<<endl;
				}			
				cout<<"symbol "<<tok1<<" = "<<i<<endl;	
			}
			else
			{
				symtable[tok1] = addr;
				cout<<"label "<<tok1<<" = "<<addr<<endl;	
				process_mnemonic();			
			}			
			}
		}
	}
	ofstream ofs("test.c8",ios::binary|ios::out|ios::trunc);
	ofs<<ss.str();
	ofs.close();
	return 0;
}