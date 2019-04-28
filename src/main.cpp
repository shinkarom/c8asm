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

void emit_twobytes(int hi, int lo)
{
	ss.write((char*)&hi,1);
	ss.write((char*)&lo,1);
}

void emit_oneword(int wrd)
{
	emit_twobytes(wrd>>8,wrd % 256);
}

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
		process_one_db();
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
		process_one_dw();		
	cout<<"processed dw"<<endl;	
}

void process_ld_i()
{
	get_token();
	expect_comma();
	auto rr = peek_token();
	if(is_registerthing(rr)){
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x55);
	} else if(is_numberthing(rr)){
		auto r = expect_address();
			emit_oneword(0xA000|r);
	} else{
		cout<<"Wrong argument."<<endl;
		exit(1);
	}
}

void process_ld_vx()
{
	const unordered_map<string, int> not_register_second = {{"dt",0x07},{"k",0x0A},{"i",0x65}};
	auto x = expect_registerthing();
	expect_comma();	
	auto y = peek_token();
	if(auto o = not_register_second.find(y);o!=not_register_second.end()){
		emit_twobytes(0xF0|x,o->second);
		get_token();
	}		
	else if(is_registerthing(y)){
		auto yy = expect_registerthing();
		emit_twobytes(0x80|x,0x00|(yy<<4));
	} else if(is_number(y)){
		auto yy = get_number();
		if(yy>255){
			cout<<"Number too large."<<endl;
			exit(1);
		}
		else emit_twobytes(0x60|x,yy);
	}
}

void process_ld()
{
	const unordered_map<string, int> not_register_first = {{"dt",0x15},{"st",0x18},{"f",0x29},{"b",0x33}};
	auto mn = peek_token();
	if(is_registerthing(mn))
		process_ld_vx();
	else if(auto o = not_register_first.find(mn);o!=not_register_first.end()){
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,o->second);		
	}
	else if(mn=="i")
		process_ld_i();
}

void process_mnemonic()
{
	const unordered_map<string,int> two_arg_mnemonics = {{"or",0x8001},{"and",0x8002},{"xor",0x8003},{"sub",0x8005},{"subn",0x8007},{"se",0x5000}};
	auto mnemonic = get_token();
	if (!is_reserved(mnemonic)) return;	
	if(auto o=two_arg_mnemonics.find(mnemonic);o!=two_arg_mnemonics.end()){
		auto x = expect_registerthing();
		expect_comma();
		auto y = expect_registerthing();
		emit_oneword(o->second|(x<<8)|(y<<4));
	}
	if(mnemonic=="db")	
		process_db();
	else if(mnemonic=="dw")
		process_dw();
	else if (mnemonic=="cls"){
		emit_twobytes(0x00,0xE0);
		addr+=2;
	}
	else if (mnemonic=="ret"){
		emit_twobytes(0x00,0xEE);
		addr+=2;		
	}	
	else if (mnemonic=="ld"){
		expect_whitespace();
		process_ld();
		addr+=2;
	}
	cout<<"mnemonic "<<mnemonic<<endl;
	auto tk = get_token();
	if(tk!=""){
		cout<<"Extraneous token "<<tk<<" on the line."<<endl;
		exit(0);
	}		
}

void process_line(string s)
{
	if(s.size()>0){
		init_tokenizer(s);
		auto tok1 = peek_token();
		if(is_reserved(tok1))			
			process_mnemonic();
		else {
			get_token();	
			if(auto tok2 = peek_token(); tok2=="equ"){		
				get_token();
				int i;
				auto tok3 = peek_token();
				if(is_number(tok3)){	
				i = get_number();
					symtable[tok1] = i;
				} else if(is_register(tok3)){
					i=get_register();
					regtable[tok1] = i;
				} else{
					cout<<"Wrong constant."<<endl;
					exit(1);
				}			
				cout<<"symbol "<<tok1<<" = "<<i<<endl;	
			}
			else {
				symtable[tok1] = addr;
				cout<<"label "<<tok1<<" = "<<addr<<endl;	
				process_mnemonic();			
			}			
		}
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
		thisline = thisline.substr(0,thisline.find(";",0));
		process_line(thisline);
	}
	ofstream ofs("test.c8",ios::binary|ios::out|ios::trunc);
	ofs<<ss.str();
	ofs.close();
	return 0;
}