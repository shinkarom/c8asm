#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

vector<string> lines;
stringstream ss(stringstream::binary|stringstream::out);

unordered_map<string, int> symtable;
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

string preprocess_line(string& arg)
{
	return arg.substr(0,arg.find(";",0));
}

void process_db(Tokenizer& t)
{
		t.expect_whitespace();
		int r;
		r= t.expect_numberthing();
		if(r>255){
			cout<<"Number too large."<<endl;
			exit(0);
		}else{
			ss.write((char*)&r,1);
			addr++;
		}
		while(t.optional_comma())
		{
			r=t.expect_numberthing();
			if(r>255){
				cout<<"Number too large."<<endl;
				exit(0);
			}else{
				ss.write((char*)&r,1);
				addr++;
			}			
		}
		cout<<"processed db"<<endl;	
}

void process_dw(Tokenizer& t)
{
	t.expect_whitespace();
	int r;
	r = t.expect_numberthing();
	if(r>65535){
		cout<<"Number too large."<<endl;
		exit(0);
	}else{
		emit_oneword(r);
		addr+=2;
	}
	while(t.optional_comma())
	{
		r=t.expect_numberthing();
		if(r>65535){
			cout<<"Number too large."<<endl;
			exit(0);
		}else{
			emit_oneword(r);
			addr+=2;
		}			
	}
	cout<<"processed dw"<<endl;	
}

void process_ld(Tokenizer& t)
{
	auto mn = t.peek_token();
	if(t.is_register(mn))
	{
		
	}
	else if(mn=="dt")
	{
		t.get_token();
		t.expect_comma();
		auto r = t.expect_register();
		emit_twobytes(0xF0|r,0x15);
	}
}

void process_mnemonic(Tokenizer& t)
{
	auto mnemonic = t.get_token();
	if (!t.is_reserved(mnemonic)) return;	
	if(mnemonic=="db"){	
		process_db(t);
	} 
	else if(mnemonic=="dw"){
		process_dw(t);
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
		t.expect_whitespace();
		process_ld(t);
		addr+=2;
	}
	cout<<"mnemonic "<<mnemonic<<endl;
	auto tk = t.get_token();
	if(tk!="")
	{
		cout<<"Extraneous token "<<tk<<" on the line."<<endl;
		exit(0);
	}		
}

void pass_1()
{
	addr = 0x200;
	for(auto l: lines)
	{
		Tokenizer t(l);
		auto tok1 = t.peek_token();
		if(t.is_reserved(tok1))			
		{ 
			process_mnemonic(t);
		}
		else 	
		{
		t.get_token();	
		if(auto tok2 = t.peek_token(); tok2=="equ")
		{		
			t.get_token();
			auto tok3 = t.get_token();
			int i = stoi(tok3);
			symtable[tok1] = i;
			cout<<"symbol "<<tok1<<" = "<<i<<endl;	
		}
		else
		{
			symtable[tok1] = addr;
			cout<<"label "<<tok1<<" = "<<addr<<endl;	
			process_mnemonic(t);			
		}			
		}
	}
}

void pass_2()
{

}

int main(int argc, char *argv[])
{
	ifstream ifs("test.asm",ios::in);
	char templine[256];
	while(ifs.getline(templine,256))
	{
		string thisline(templine);
		thisline = preprocess_line(thisline);
		if(thisline.size()>0)
		{
			lines.push_back(thisline);
		}
	}
	pass_1();
	pass_2();
	ofstream ofs("test.c8",ios::binary|ios::out|ios::trunc);
	ofs<<ss.str();
	ofs.close();
	return 0;
}