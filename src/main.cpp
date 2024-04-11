#include "../include/Kernel.h"
#include "../include/INode.h"
#include<cstring>
#include<vector>

#define ERROR "\033[1;31m[ERROR]\033[0m "
#define WARNING "\033[1;33m[WARNING]\033[0m "
#define INFO   "\033[1;34m[INFO]\033[0m "


    /* 解析交互命令 */
	static vector<char*> parseCmd(char* s)
	{
		char* p = s, * q = s;
		vector<char*> result;
		while (*q != '\0')
		{
			if (*p == ' ')
			{
				p++;
				q++;
			} 
			else
			{
				while (*q != '\0' && *q != ' ') 
				{
					q++;
				}
				char* newString = new char[q - p + 1];
				for (int i = 0; i < q - p; i++) 
				{
					newString[i] = *(p + i);
				}
				newString[q - p] = '\0';
				result.push_back(newString);
				p = q;
			}
		}
		return result;
	}





int main()
{
	Kernel* k = Kernel::getInstance();   /* 获取内核实例 */
	k->initialize();                     /* 内核初始化 */
	
	
	
	
	cout<< "====================================" << endl;
    cout<< "\033[1;33mUNIX V6++二级文件系统 by 2152095 龚宣\033[0m" << endl;
    cout<< "====================================" << endl;
    cout << "输入\033[1;33mhelp\033[0m查看所有函数信息与输入格式 ..." << endl;
    cout<< "====================================" << endl;

	while (true)
	{

   
	    cout << "\033[1;32mroot@MySFS \033[0m:\033[1;34m " << k->curdir << "\033[0m$";
		char input[100000];
		cin.getline(input, 100000);
		//cout<<88<<endl;
		//cout<<input<<endl;
		vector<char*> result = parseCmd(input);  /* 解析交互命令 */
		//cout<<77;
		if (result.size() > 0)
		{
			if (strcmp(result[0], "help") ==0)
			{
				if(result.size()==1){
				cout << "[Commands provided]:" << endl;
				cout << "help                                   : 显示功能总览" << endl;
				cout << "fformat                                : 格式化文件卷" << endl;
				cout << "ls                                     : 当前路径下的文件" << endl;
				cout << "cd <dirname>                           : 改变当前路径" << endl;
				cout << "mkdir <dirname>                        : 创建文件夹" << endl;
				cout << "rm <dirname>                           : 删除文件(夹)" << endl;
				cout << "fcreate <filename> <mode>              : 新建文件" << endl;
				cout << "fopen <filename>  <mode>               : 打开文件" << endl;
				cout << "fclose <fd>                            : 关闭文件" << endl;
				cout << "fread <fd> <nbytes>                    : 读文件" << endl;
				cout << "fwrite <fd> <string>                   : 写文件" << endl;
				cout << "flseek <fd> <offset> <whence>          : 移动文件读写指针" << endl;
				cout << "cp <file1> <file2> <mode>              : 拷贝文件内容" << endl;
				cout << "frename <file1> <file2>                : 重命名文件" << endl;
				cout << "ftree <dirname>                        : 显示目录树" << endl;
				cout << "pwd                                    : 显示当前路径" << endl;
				cout << "exit                                   : 退出" << endl;
				cout << "输入 help [funcname] 查看函数具体说明.        " << '\n';
				cout << endl;
				}
				else{
				
        if(strcmp(result[1], "fformat") ==0){
            cout << "\033[1;33m格式化文件卷\033[0m" << endl;
            cout << "fformat" << endl;
		}
       else if(strcmp(result[1], "ls") ==0){
            cout<< "\033[1;33m显示当前目录下所有内容\033[0m" << endl;
            cout<< "ls" << endl;
	   }
        else if(strcmp(result[1], "cd") ==0){
            cout<< "\033[1;33m改变当前工作目录\033[0m" << endl;
            cout<< "cd [dirname]" << endl;
            cout<< "(const char*) dirname: 路径" << endl;
		}
        else if(strcmp(result[1], "mkdir") ==0){
            cout<< "\033[1;33m创建文件夹\033[0m" << endl;
            cout<< "mkdir [dirname]" << endl;
            cout<< "(const char*) dirname: 文件夹名" << endl;
		}
        else if(strcmp(result[1], "rm") ==0){
            cout<< "\033[1;33m删除文件(夹),比fdelete多了删除文件夹的功能\033[0m" << endl;
            cout<< "rm [filename]" << endl;
            cout<< "(const char*) filename: 要删除的文件(夹)名" << endl;
		}
		else if(strcmp(result[1], "fcreate") ==0){
            cout << "\033[1;33m创建文件\033[0m" << endl;
            cout << "fcreate [filename] [mode]" << endl;
            cout << "(const char*) filename: 要创建的文件的文件名" << endl;
            cout << "(int) mode: 文件模式，缺省IRWXU(可读可写可执行)" << endl;
	   }
         else if(strcmp(result[1], "fopen") ==0){
            cout<< "\033[1;33m打开文件\033[0m" << endl;
            cout<< "fopen [filename] [mode]" << endl;
            cout<< "(const char*) filename: 要打开的文件名" << endl;
            cout<< "(int) mode: 打开模式,缺省为只读" << endl;
		 }
       else if(strcmp(result[1], "fclose") ==0){
            cout<< "\033[1;33m关闭文件\033[0m" << endl;
            cout<< "fclose [fd]" << endl;
            cout<< "(int) fd: 要关闭的文件的文件描述符" << endl;
	   }
      else if(strcmp(result[1], "fread") ==0){
            cout << "\033[1;33m读取文件\033[0m" << endl;
            cout << "fread [fd] [length]" << endl;
            cout << "(int) fd: 要读取的文件的文件描述符" << endl;
            cout << "(int) length: 读取长度" << endl;
      }
      else if(strcmp(result[1], "fwrite") ==0){
            cout<< "\033[1;33m写入文件\033[0m" << endl;
            cout<< "fwrite [fd] [contents]" << endl;
            cout << "(int) fd: 要写入的文件的文件描述符" << endl;
            cout << "(const char*) contents: 要写入的内容" << endl;
	  }
       else if(strcmp(result[1], "flseek") ==0){
            cout<< "\033[1;33m移动文件指针\033[0m" << endl;
            cout<< "flseek [fd] [offset] [whence]" << endl;
            cout<< "(int) fd: 文件的文件描述符" << endl;
            cout<< "(int) offset: 文件指针移动偏移量" << endl;
            cout<< "(int) whence: 0->从文件开头 1->从当前位置 2->从文件结尾" << endl;
	   }
	 else if(strcmp(result[1], "cp") ==0){
		   cout<< "\033[1;33m复制当前目录下的文件到另一个文件\033[0m" << endl;
            cout<< "cp <file1> <file2>"<< endl;
            cout<< "(const char*) file1: 被拷贝的文件名(source)" << endl;
            cout<< "(const char*) file2: 拷贝到的文件名(destination)" << endl;
			cout<<"(int) mode:0:[外部-> 磁盘] 1:[磁盘->外部] 2:[磁盘->磁盘]"<<endl;
			//cout<<"这里表示磁盘文件要加地址前缀\033[1;35mroot@MySFS:\033[0m,本地文件不用加"<<endl;
	 } else if(strcmp(result[1], "frename") ==0){
		   cout<< "\033[1;33m重命名文件\033[0m" << endl;
            cout<< "frename <file1> <file2>"<< endl;
            cout<< "(const char*) file1: 原文件名" << endl;
            cout<< "(const char*) file2: 要修改的文件名" << endl;
	 }
	 else if(strcmp(result[1], "ftree") ==0){
		   cout<< "\033[1;33m显示目录树\033[0m" << endl;
            cout<< "ftree <dirname>"<< endl;
            cout<< "(const char*) dirname: 目录名" << endl;
	 }
	  else if(strcmp(result[1], "pwd") ==0){
		   cout<< "\033[1;33m显示当前路径\033[0m" << endl;
            cout<< "pwd"<< endl;
	 }
       else if(strcmp(result[1], "exit") ==0){
            cout << "\033[1;33m退出程序\033[0m" << endl;
            cout << "exit" << endl;
          }
       else{
		 
            cout<< "不存在此函数，请重新输入." << endl;
          
					
				}
		}
			}
			else if (strcmp(result[0], "cd") == 0)
			{
				if (result.size() == 2)
				{
					if(strcmp(result[1], ".") ==0){
                        //仍然停留在原目录
					}
					else if(strcmp(result[1], "..") ==0)
					{
						char* padir = k->curdir;
						if(padir[1]=='\0')
						{
							cout<<WARNING;
							cout << "Already in the root!" << endl;
						}
						else
						{
							for (int i = 127; i >= 0; i--) /* 从结尾开始找最后一个/ */
							{
								if (padir[i] != '/')
								{
									padir[i] = '\0';
								}
								else
								{
									if(i!=0)
									{
										padir[i] = '\0';
									}
									break;
								}
							}
							k->cd(padir);
							if (k->error == Kernel::NOTDIR)
								cout <<ERROR<< padir << ": Not a directory" << endl;
							else if (k->error == Kernel::NOENT)
								cout <<ERROR<< padir << ": No such file or directory" << endl;
						}
					}
					else
					{
						
						k->cd(result[1]);
						if (k->error == Kernel::NOTDIR)
							cout <<ERROR<< result[1] << ": Not a directory" << endl;
						else if (k->error == Kernel::NOENT)
							cout <<ERROR<< result[1] << ": No such file or directory" << endl;
					}
				}
				else
					cout <<ERROR<< "Wrong num of parameters" << endl;
			}
			else if (strcmp(result[0], "fformat") == 0)
			{
				cout<<WARNING<<"\033[1;31mAll of your data will be released, do you want to Continue? [Y/N]\033[0m"<<endl;
				char ch[1024];
				cin.getline(ch,1024);//linux的 换行符与windows不同，要注意
				if(ch[0]=='Y'||ch[0]=='y'){
				k->format();
				k->initialize();
				cout<<INFO<<"Format Successfully Completed!!!"<<endl;
				}
			}
			else if (strcmp(result[0], "mkdir") == 0)
			{
				if (result.size() ==2)
				{
					k->mkdir(result[1]);
					
					if (k->error == Kernel::NOENT)
						cout <<ERROR<< "No such a file or directory" << endl;
					if (k->error == Kernel::ISDIR)
						cout <<ERROR<< result[1] << ": This directory already exist" << endl;
				}
				else{
					
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}

			}
			else if (strcmp(result[0], "ls") == 0)
			{
				cout<<"\033[1;34m"; 
				k->ls();
				cout<<"\033[0m";
			}
			else if (strcmp(result[0], "fopen") == 0)
			{
				if (result.size()==2||result.size()==3)//默认只读模式打开
				{
					int mode;
					if(result.size()==2){
						mode=File::FREAD;
					}
					else{
						mode=atoi(result[2]);
					}
					const int fd = k->open(result[1], mode);
					if (k->error == Kernel::NO_ERROR)
						cout << "文件句柄fd = " << fd << endl;
					else if (k->error == Kernel::ISDIR)
						cout <<ERROR<< result[1] << ": This is a directory" << endl;
					else if (k->error == Kernel::NOENT)
						cout <<ERROR<< result[1] << ": No such a file or directory" << endl;
					else if (k->error ==Kernel::NOACCESS)
					    cout <<ERROR<< result[1] << ": No Access" << endl;
				}
				else
					cout <<ERROR<< "Wrong num of parameters" << endl;

			}
			else if (strcmp(result[0], "fcreate") == 0)
			{
				int fd;
				if (result.size() ==2||result.size()==3)
				{
					int mode;
					if(result.size()==2){
						mode=Inode::IRWXU;
                        
					}
					else{
						mode=atoi(result[2]);
					}
					fd = k->create(result[1], mode);

					if (k->error == Kernel::ISDIR)
						cout <<ERROR<< result[1] << ": This is a directory" << endl;
					if (k->error == Kernel::NOENT)
						cout <<ERROR<< result[1] << ": No such a file or directory" << endl;
					if (k->error == Kernel::NO_ERROR)
						cout << "文件句柄fd = " << fd << endl;
				}
				else
					cout <<ERROR<< "Wrong num of parameters" << endl;

			}
			else if (strcmp(result[0], "fclose") == 0)
			{
				if (result.size() ==2)
					k->close(atoi(result[1]));
				else
					cout <<ERROR<< "Wrong num of parameters" << endl;
			}
			else if (strcmp(result[0], "fread") == 0)
			{
				if (result.size() == 3) {
					char* buf = new char[atoi(result[2])];
					buf[0] = '\0';
					int actual = k->fread(atoi(result[1]), buf, atoi(result[2]));
					if (actual == -1)
					{
						if (k->error == Kernel::BADF)
							cout << atoi(result[1]) << ":This fd is wrong" << endl;
						else if(k->error==Kernel::NOACCESS)
						    cout <<ERROR<< result[1] << ": No Access" << endl;
					}
					else
					{
						if (actual > 0)
						{
							for (int i = 0; i < actual; i++)
							{
								cout << buf[i];
							}
							cout << endl;
						}
						cout <<INFO<< "Successfully read " << actual << " bytes" << endl;
					}
					delete buf;
				}
				else
					cout <<ERROR<< "Wrong num of parameters" << endl;
			}
			else if (strcmp(result[0], "fwrite") == 0)
			{
				//cout<<11;
				if (result.size() == 3)
				{
					int len=strlen(result[2]);
					//cout<<11<<endl;
					int actual = k->fwrite(atoi(result[1]), result[2], len);
					//cout<<22<<endl;
					if (actual == -1)
					{
						if (k->error == Kernel::BADF)
							cout << atoi(result[1]) << ": This fd is wrong" << endl;
						else if(k->error==Kernel::NOACCESS)
						    cout <<ERROR<< result[1] << ": No Access" << endl;
					}
					else
					{
						cout<<INFO << "Successfully write " << actual << " bytes" << endl;
					}
				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "flseek") == 0)
			{
				if (result.size() == 4)
				{
					if (atoi(result[3]) >= 0 && atoi(result[3]) <= 2)
					{
						k->fseek(atoi(result[1]), atoi(result[2]), atoi(result[3]));
						if (k->error == Kernel::BADF)
							cout <<ERROR<< atoi(result[1]) << ": This fd is wrong" << endl;
					}
					else
						cout <<ERROR<< result[3] << ": This whence is wrong" << endl;

				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "rm") == 0)
			{
				if (result.size() ==2)
				{
					k->fdelete(result[1]);
					if (k->error == Kernel::NOENT)
						cout <<ERROR<< result[1] << ": No such a file or directory" << endl;
                    if(k->error==Kernel::E_BUSY)
					    cout <<ERROR<< result[1] << ": Device or resource busy" << endl;
				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "cp") == 0)
			{
				if (result.size()==4)
				{
					if(atoi(result[3]) >= 0 && atoi(result[3]) <= 2){
					//mode==0 外部-> 磁盘 mode==1 磁盘->外部  mode==2 磁盘->磁盘
					k->cp(result[1], result[2],atoi(result[3]));
					if (k->error == Kernel::NOENT){
						
						cout <<ERROR<< result[1] << ": No such a file or directory" << endl;
					}
					else if (k->error == Kernel::NOOUTENT)
						cout <<ERROR<< result[1] << ": No such a file or directory" << endl;
					else if (k->error == Kernel::ISDIR)
						cout <<ERROR<< result[1] <<"or"<<result[2] << ": This is a directory" << endl;
					}
					else{
						cout <<ERROR<< result[3]  << ": No such mode (0~2)" << endl;
					}
				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "frename") == 0)
			{
				if (result.size() ==3)
				{
					k->frename(result[1], result[2]);
					if (k->error == Kernel::NOENT)
						cout <<ERROR<< result[2] << ": No such a file or directory" << endl;
				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "ftree") == 0)
			{
				if (result.size() == 2)
				{
					string path = result[1];
					k->ftree(path);
				}
				else
				{
					cout <<ERROR<< "Wrong num of parameters" << endl;
				}
			}
			else if (strcmp(result[0], "exit") == 0)
			{
				k->clear();
				break;
			}
			else if (strcmp(result[0], "pwd") == 0)
			{
				cout << k->curdir << endl;
			}
			else {
				cout <<ERROR<< "command \'" << result[0] << "\' not found" << endl;
			}
		}
	}
	return 0;
}

