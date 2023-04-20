//?// community_network_creater.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#include "pch.h"
//#pragma warning(disable:4996)
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <algorithm>
#include <vector>
#include <exception>
#include <list>
#include <assert.h>
#include <windows.h>
#include <cstdio>
#include <map>
#include <queue>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include <string>
#include <io.h>
#define RUN_TIME 5 

using namespace std;
struct Edge {//表示一条边的结构体 
	int a, b;
};

//=====================和显示有关的全局=====================
const int LINKSHOW = 5000;//连了多少个节点才输出一下进度


//=================基础的全局==================================
double link_pro = 0.00035;//随机网络的连接概率
int NODE_NUM = 5000;//number of node 网络节点数目
int COMMUNITY_SIZE = 250;// 社团大小
int SOC_NUM = NODE_NUM / COMMUNITY_SIZE;//社团数量
const int MAX_NODE_NUM = 500000;
const int MAX_SOC_NUM = 50000;
vector<int> edge[MAX_NODE_NUM];		//edge[i]放着i节点所有的边
vector<int> group[MAX_SOC_NUM];  //group[i]放着第i个社团里所拥有的的成员节点
vector<int> community_each_node_in[MAX_NODE_NUM];//记录每个节点所在的社团
const int max_num_of_community_a_node_in = 1;//一个节点最多处在多少个社团
int num_of_edges = 0;//记录边的数目
vector<Edge> all_edges;//存放所有的边，用于SCP(在SCP是数组形式的，注意类型)

FILE* result;//记录运行结果的文件指针 
double z=1.0;//zipf shape parameter value
//====================构造社团结构网络的全局（by梁萍佳）================
const double IN_PRO = 0.9;//社团内连接概率
const int OUT_LINK = 10;//连接社团外的平均节点数

//=========================CDN算法的全局变量和函数============================
#define _CRT_SECURE_NO_WARNINGS


//----------------下面是正文变量------------------------------------------------------------------------------------
const int MAX_FILE_NUM = 10000000;
struct FileCache{
	int filenum;
	int time;
	FileCache(int f=-1,int t=0){
		filenum=f;
		time=t;
	}
}; 
int FILE_NUM = 50000;//number of file 文件数目
int DEMAND_NUM = 500000;//number of requests per node 总共发出的请求数
const int CACHE_SIZE = 5;//每个节点的大小，暂时当是一样为5
enum CACHE_STRATEGY { LCE, LCD, SOC, SOCD,SOC2,SOCD2 };//缓存的策略
vector<FileCache> file[MAX_NODE_NUM];		//file[i]放着i节点所有的缓存文件
int hit_times;				//命中缓存的次数
int copy_times;				//复制次数 
int replace_times;          //替换次数（即满了之后再替换的次数） 
int time_count[MAX_NODE_NUM];//time_count[i]代表用时i的请求次数 
double popularity[MAX_FILE_NUM];//文件流行度，也就是要这个文件的概率
double addup[MAX_FILE_NUM];//addup[k]=小于k的文件序号的需求度之和
int source[MAX_FILE_NUM];//文件的源服务器
//----------------下面是函数声明------------------------------------------------------------------------------------
int rand_file();//依概率随机生成访问的文件，返回的就是文件序号
double drand();//return a random real number between 0 and 1
void reinitialize();//after calling object1,use this function to reinitialize.
void refresh(int fnum, int n);
void zipf_pdf(double z, double lambda[]);//zipf file popularity distribution with shape parameter z
double object1(double popularity[], vector<int>edge[], vector<FileCache>file[], CACHE_STRATEGY strategy);//
//----------------跟输出信息相关的变量，不用在意--------------------------------------------------------
const int  SHOW = 0;//是否显示每个请求的信息
const int ROUTE = 0;//是否显示路径
const int DEMANDSHOW = DEMAND_NUM/10;//完成了多少个请求才输出一下进度


//====================SCP算法的全局变量==========================

const int offset = 5;//Node的哈希表长为度的十分之一
#define hash_list vector<int>
#define size_type int

//======================和重连相关的类和全局变量=============================
//把每个社团看成一个节点，构建一个带权图，边的权值表示社团间的边的数量,用edge_of_com_graph
vector<int> edge_of_com_graph[MAX_SOC_NUM];//社团图的邻接表
vector<int> weight_of_edge_of_com_graph[MAX_SOC_NUM];//对应边的权重（与edge_of_com_graph一起用）
const int com_graph_hash_list_length = 100;//哈希表长
vector<Edge> edge_outside_community;//社团外的边，所有社团间的边的集合


struct com_graph_neighbour {
	int com_num;
	int index;
};
class no_such_node : public exception {//com_graph_hash删除节点时，如果找不到节点则抛出这个异常
public:
	const char* what()throw() {
		return "\n";
	}
};
//这个类用来实现快速查找社团图节点的一个邻居在edge_of_com_graph中的下标
class com_graph_hash {
public:
	com_graph_hash();
	int find_index(int com_num);//查找编号为com_num的社团，返回它的下标，找不到返回-1
	void insert_neighbor(int com_num, int index);
	void reset_com_graph(const vector<int> new_node);//重新装入一个新的社团图节点
	void delete_node(int node)throw(no_such_node);//删除一个节点
	void clear();//清空，vector用自带的clear清空
	//void write_back_to_vector(vector<int> edge_of_com_graph[SOC_NUM], vector<int> weight_of_edge_of_com_graph[SOC_NUM]);//鎶婂搱甯岃〃涓�鐨勫唴瀹瑰啓鍥炲埌杩欏叏灞€鍙橀噺涓�
private:
	vector<com_graph_neighbour> com_hash_table[com_graph_hash_list_length];//鍝堝笇琛�
	static int hash(int com_num);
};
com_graph_hash com_nodes[MAX_SOC_NUM];//哈希表存放的社团图邻接表

class Node {//哈希表服务 
public:
	Node();
	Node(const Node& other);
	Node(const size_type * array, int size, size_type name);
	Node(int degree, size_type name);
	~Node();
	size_type name()const;
	void setName(size_type new_name);
	//size_type& at(int index1,int index2)throw(hash_table_out_of_rang_exception);
	//hash_list& hash_list_at(int index)throw(hash_table_out_of_rang_exception);
	int degree()const;
	void setDegree(int new_degree);
	void insert_neighbor(const size_type& new_neighbor);
	bool is_neighbor(const size_type& target)const;
	void test_display()const;
	vector<size_type> common_neighbor(const Node& other)const;
	void clear_neighbor();
private:
	hash_list * neighbor_hash_table;
	size_type node_name;
	static int hash(const size_type& neighbor, int hash_table_size);
	int _degree;//搴︽槸鏋勯€犳椂纭�绔嬬殑锛岀敤浜庣‘瀹氬搱甯岃〃鐨勫ぇ灏忥紝鎻掑叆閭诲眳鏃朵笉淇�鏀�
};


void connect_social(vector<int>edge[], double inner_pro, double outside_pro);//鎼炲嚭涓€涓�鏈夌ぞ鍥㈢粨鏋勭殑缃戠粶

//=================================重连相关函数======================== 
void clear_edges();//清空所有的边(edge,num_of_edges,all_edges)
void clear_all(); //清空还原所有全局变量
void set_community_each_node_in();//把group中的信息拷贝到community_each_node_in中
void assign();//通过随机给节点选择1 -- max_num_of_community_a_node_in个社团，实现把节点随机分配到社团中
void link_one_edge(int node1, int node2);//连接两个节点, 没有检查两个节点是否有连边的步骤，调用之前要确保不会引起边的重复 
void link_all_edge(double pro);//按照概率pro在所有节点之间随机连边，生成一个随机网络
bool is_neighbour(int node1, int node2);//简单的遍历查找，要高效的话用node存节点可以哈希查找
bool is_in_one_community(int node1, int node2);//判断两个节点是否在同一个社团
double Q_function();//模块度Q函数，经典版 Q = (1/2m)*求和[(Aij-di*dj/2m)*delta(i,j)]
//Aij为节点i，j间有连边，di,dj是度，m是边总数，delta(i,j)是i,j是否在同一个社团
bool is_in_this_community(int node, int com);//判断一个节点是否属于某个社团
vector<int> find_common_community(int node1, int node2);//找出两个节点共同在的社团
void build_com_graph();//构建社团图,社团图用来判断网络的连通性，目的是降低判断连通性的复杂度，但是好像为此折腾了好多东西
void display_com_graph() ;//输出社团图邻接表（测试用）
void get_edge_outside_community();//取社团外的边放到 edge_outside_community中 
bool is_connected();//利用社团图判断网络的连通性,用广搜
bool is_connected2() ;//直接全网络广搜判断连通性
void guarantee_network_connected();//通过增加少量边（如果网络过于稀疏可能会加很多）保证网络的连通性 
void guarantee_community_is_connected();//检查每个社团内部的连通性，（用SCP划分得到的社团可以保证连通，不需要检查，随机网络的需要），如果不连通时通过随机加边使之连通
void reconnect(int num_of_edges_to_change);//随机地把数目为num_of_edges_to_change的社团外的边重连到社团内，以加强社团强度


void CDN();//在当前全局变量表示的网络下运行CDN算法（6策略） 
string getName(string origin_name);

int main()
{
	cin>>z;
	string name="t_v3_result.txt";
	name=getName(name.c_str());
	result = fopen(name.c_str(),"w"); 
	
	fprintf(result,"**********************************************************\n");
	fprintf(result,"NODE_NUM = %d COMMUNITY_SIZE = %d link_pro = %lf\nFILE_NUM = %d DEMAND_NUM = %d\n",NODE_NUM,COMMUNITY_SIZE,link_pro,FILE_NUM,DEMAND_NUM);
	//开始跑 
	fprintf(result,"==============================\n");
	srand((unsigned int)time(NULL));
	cout << "==========================\n";
	cout << "Linking the network...\n";
	link_all_edge(link_pro);//构建随机网络 
				
	cout << "link successfully" << endl;
	cout << "==========================\n";
			
	//connect_social(edge,IN_PRO, (double)OUT_LINK / NODE_NUM);
	cout << "edges:" << num_of_edges << endl;
	assign();//把节点随机分配到社团中 
	guarantee_network_connected();//加少量边以保证网络连通 
	guarantee_community_is_connected();//加少量边以保证每个社团内部连通 
						
	
	cout << "\nedges:" << num_of_edges << endl;
	fprintf(result,"edges : %d average degree: %lf Q = %lf\n",num_of_edges,2*(double)num_of_edges/(double)NODE_NUM,Q_function());
	printf("edges : %d average degree: %lf\n",num_of_edges,2*(double)num_of_edges/(double)NODE_NUM);
	//SCP();
	//build_com_graph();//创建社团图，方便重连 
	//get_edge_outside_community();//都是重连前的必要处理 
	//cout << "oustside edge:" << edge_outside_community.size() << endl;
	//fprintf(result,"outside edges: %d\n",edge_outside_community.size());
	fprintf(result,"Z\tLCE     \t        \t    \t       \tLCD     \t        \t    \t       \tSOC     \t        \t    \t       \tSOC2     \t        \t   \t      \tSOCD    \t        \t     \t       \tSOCD2   \t        \t    \t       \n");
	fprintf(result," \thit_rate\tresponse\tcopy\treplace\thit_rate\tresponse\tcopy\treplace\thit_rate\tresponse\tcopy\treplace\thit_rate\tresponse\tcopy\treplace\thit_rate\tresponse\tcopy\treplace\thit_rate\tresponse\tcopy\treplace\n");
	//int edge_to_change = edge_outside_community.size()/10;//每次重连十分之一的边 
	//for(int ii = 0;ii < 9;ii++){
	//	reconnect(edge_to_change);
	//}
	//edge_to_change = edge_outside_community.size()/10;
	//for(int run_times = 1;run_times <= 10;++run_times){
		//fprintf(result,"reconnect %d edges\n",edge_to_change);
		/*if (edge_to_change == -1) {
			break;
		}
		else {
			reconnect(edge_to_change);//把部分社团外的边改到社团内 
			
			cout << "oustside edge:" << edge_outside_community.size() << endl;
			CDN();//运行CDN 
		}	*/
	//}
	fprintf(result,"%f\t",z);	
	CDN();
	clear_all();//清空还原所有全局变量 
	fprintf(result,"========================================\n");
	fclose(result);
	system("pause");
	
	return 0;
}

#define NAME_LEN 500
string getName(string origin_name) {
	int pos = -1, i;
	for (i = origin_name.length(); i >= 0; i--) {//找出最后一个'.'的位置 
		if (origin_name[i] == '.') {
			pos = i;
			break;
		}
	}
	char pre[NAME_LEN], suf[NAME_LEN], newname[NAME_LEN];//前 后缀 , 后缀包括. 
	if (pos == -1)	pos = origin_name.length();//找不到点就当点在最末 
	if (pos >= 0) {//有至少一个 . 
		for (i = 0; i<pos; i++)pre[i] = origin_name[i];
		pre[i] = '\0';
		for (i = 0; origin_name[i + pos] != '\0'; i++)suf[i] = origin_name[i + pos];
		suf[i] = '\0';
	}
	strcpy(newname, origin_name.c_str());
	i = 0;
	while (_access(newname, 0) == 0) {//这个名字的文件存在 
		i++;
		sprintf(newname, "%s(%d)%s", pre, i, suf);
	}
	return newname;
}

//把group中的信息拷贝到community_each_node_in中
void set_community_each_node_in() {
	for (int i = 0; i < SOC_NUM; ++i) {
		for (vector<int>::iterator it = group[i].begin(); it < group[i].end(); ++it) {
			community_each_node_in[*it].push_back(i);
		}
	}
}

//对社团节点进行连接
//加入了对全局all_edge,num_of_edges,community_each_node_in的修改
void connect_social(vector<int>edge[], double inner_pro, double outside_pro) {
	int i, j, t, mark;
	for (i = 0; i < NODE_NUM; i++) {
		if (i % LINKSHOW == 0)printf("link %d\n", i);//输出进度
		group[i / COMMUNITY_SIZE].push_back(i);//顺序分社团（0~COMMUNITY_SIZE-1为第一个社团，以此类推）
		//连接社团内的节点
		for (j = i + 1; j < (i / COMMUNITY_SIZE + 1) * COMMUNITY_SIZE; j++) {//在同一个社团内
			if (rand() % 1000 / 1000.0 < inner_pro) {//随机概率小于给定值，就要连
				mark = 0;
				for (t = 0; t < edge[i].size(); t++) {//遍历检查是否连过
					if (edge[i][t] == j) {//已经连过，标记mark=1
						mark = 1;
						break;
					}
				}
				if (mark == 0) {//没有连接过，连起来
					Edge new_edge;
					new_edge.a = i;
					new_edge.b = j;
					all_edges.push_back(new_edge);
					num_of_edges++;
					edge[i].push_back(j);
					edge[j].push_back(i);
				}
			}
		}
		//连接社团外的节点（连通）
		for (j = (i / COMMUNITY_SIZE + 1) * COMMUNITY_SIZE; j < NODE_NUM; j++) {//遍历社团外所有节点
			if (rand() % 1000 / 1000.0 < outside_pro) {//随机概率小于给定值，就要连
				mark = 0;
				for (t = 0; t < edge[i].size(); t++) {//遍历检查是否连过
					if (edge[i][t] == j) {//已经连过，标记mark=1
						mark = 1;
						break;
					}
				}
				if (mark == 0) {//没有连接过，连起来
					Edge new_edge;
					new_edge.a = i;
					new_edge.b = j;
					all_edges.push_back(new_edge);
					num_of_edges++;
					edge[i].push_back(j);
					edge[j].push_back(i);
				}
			}
		}
	}
	set_community_each_node_in();//修改community_each_node_in
}

//==============================================================================================================================================
//重连程序
//=====================================================================================================================================================


//通过随机给节点选择1 -- max_num_of_community_a_node_in个社团，实现把节点随机分配到社团中
//修改的全局：community_each_node_in,group
void assign() {
	cout << "Randomly assign nodes to communities...\n";
	for (int i = 0; i < NODE_NUM; ++i) {
		cout << "\r     \r" << i * 100 / (NODE_NUM-1) << "%";
		int rand_num = rand() % 10000000;//鑾峰緱涓€涓�寰堝ぇ鐨勯殢鏈烘暟
		int num_of_com = (rand_num%max_num_of_community_a_node_in) + 1;//随机确定节点所在的社团数
		for (int j = 0; j < num_of_com; ++j) {//随机选择社团加入
			int rand_num_2 = rand() % 10000000;//鍐嶆�¤幏寰椾竴涓�寰堝ぇ鐨勯殢鏈烘暟
			int rand_com = rand_num_2%SOC_NUM;//随机获取社团编号
				//加入社团
			community_each_node_in[i].push_back(rand_com);
			group[rand_com].push_back(i);
		}
	}
	cout << endl;
}

//连接两个节点，修改的全局变量：num_of_edges, all_edge,edge, 没有检查两个节点是否有连边的步骤，调用之前要确保不会引起边的重复
//修改的全局：all_edge,edge,num_of_edges
void link_one_edge(int node1, int node2) {
	num_of_edges++;
	Edge new_edge;
	new_edge.a = node1;
	new_edge.b = node2;
	all_edges.push_back(new_edge);
	edge[node1].push_back(node2);
	edge[node2].push_back(node1);
}

//按照概率pro在所有节点之间随机连边，生成一个随机网络
//修改的全局：all_edge,edge,num_of_edges
void link_all_edge(double pro) {
	cout << "pro = " << pro << "  Randomly connecting...\n";
	for (int i = 0; i < NODE_NUM; ++i) {
		cout << "\r     \r" << i * 100 / (NODE_NUM-1) << "%";
		for (int j = i+1; j < NODE_NUM; ++j) {//鍙屽眰寰�鐜�姣忓�硅妭鐐瑰彧閬嶅巻涓€娆★紝纭�淇濅笉浼氭湁閲嶅�嶈繛杈�
			bool to_connect = (drand() < pro);
			if (to_connect) {
				link_one_edge(i, j);
			}
		}
	}
	cout << endl;
}

//简单的遍历查找，要高效的话用node存节点可以哈希查找
//使用的全局（不修改）：edge
bool is_neighbour(int node1, int node2) {
	if (edge[node1].size() > edge[node2].size()) {
		for (vector<int>::iterator it = edge[node2].begin(); it < edge[node2].end(); it++) {
			if (*it == node1) {
				return true;
			}
		}
	}
	else {
		for (vector<int>::iterator it = edge[node1].begin(); it < edge[node1].end(); it++) {
			if (*it == node2) {
				return true;
			}
		}
	}
	return false;
}
//delta函数，判断两个节点是否在同一个社团
//使用的全局（不修改）：communty_each_node_in
bool is_in_one_community(int node1, int node2) {
	for (vector<int>::iterator it1 = community_each_node_in[node1].begin(); it1 < community_each_node_in[node1].end(); ++it1) {		
		for (vector<int>::iterator it2 = community_each_node_in[node2].begin(); it2 < community_each_node_in[node2].end(); ++it2) {
			if (*it1 == *it2) {
				return 1;
			}
		}
	}
	return 0;
}
//模块度Q函数，经典版 Q = (1/2m)*求和[(Aij-di*dj/2m)*delta(i,j)]
//Aij为节点i，j间有连边，di,dj是度，m是边总数，delta(i,j)是i,j是否在同一个社团
//使用的全局（不修改）：communty_each_node_in,edge,num_of_edges
double Q_function() {
	cout << "Compute Q...\n";
	double result = 0;
	Node* nodes = new Node[NODE_NUM];//用哈希表存邻居方便查找
	for (int i = 0; i < NODE_NUM; ++i) {
		nodes[i].setName(i);
		nodes[i].setDegree(edge[i].size());
		for (vector<int>::iterator it = edge[i].begin(); it < edge[i].end(); it++) {
			nodes[i].insert_neighbor(*it);
		}
	}
	
	for (int i = 0; i < NODE_NUM; ++i) {
		cout << "\r     \r" << i * 100 / (NODE_NUM-1) << "%";
		for (int j = i; j < NODE_NUM; ++j) {
			if (is_in_one_community(i, j)) {
				result += nodes[i].is_neighbor(j) - (double)(edge[i].size()*edge[j].size()) / (2.0*(double)num_of_edges);
			}
		}
	}
	cout << "\rq = " << result * (1.0 / (double)num_of_edges) << endl;;
	return result * (1.0 / (double)num_of_edges);
}

//===================================================================
//建立社团图，即以社团为节点的带权图
//===================================================================
com_graph_hash::com_graph_hash() {//默认构造函数
	
}
int com_graph_hash::find_index(int com_num) {//查找编号为com_num的社团，返回它的下标，找不到返回-1
	int pos = hash(com_num);
	for (vector<com_graph_neighbour>::iterator it = com_hash_table[pos].begin(); it < com_hash_table[pos].end(); ++it) {
		if (it->com_num == com_num) {
			return it->index;
		}
	}
	return -1;
}
void com_graph_hash::insert_neighbor(int com_num, int index) {
	int pos = hash(com_num);
	com_graph_neighbour new_neighbor;
	new_neighbor.com_num = com_num;
	new_neighbor.index = index;
	com_hash_table[pos].push_back(new_neighbor);
}
int com_graph_hash::hash(int com_num) {
	int p = 16777619;
	int _hash = (int)2166136261L;
	_hash = (_hash ^ com_num) * p;
	_hash += _hash << 13;
	_hash ^= _hash >> 7;
	_hash += _hash << 3;
	_hash ^= _hash >> 17;
	_hash += _hash << 5;
	if (_hash < 0) {
		_hash *= -1;
	}
	return _hash % com_graph_hash_list_length;
}
void com_graph_hash::reset_com_graph(const vector<int> new_node) {
	for (int j = 0; j < com_graph_hash_list_length; ++j) {
		com_hash_table[j].clear();
	}
	int index = 0;
	for (vector<int>::const_iterator it = new_node.begin(); it < new_node.end(); ++it) {
		insert_neighbor(*it,index);
		index++;
	}
}
void com_graph_hash::delete_node(int node)throw(no_such_node) {
	int pos = hash(node);
	for (vector<com_graph_neighbour>::iterator it = com_hash_table[pos].begin(); it < com_hash_table[pos].end(); ++it) {
		if (it->com_num == node) {
			com_hash_table[pos].erase(it);
			return;
		}
	}
	throw no_such_node();
}
void com_graph_hash::clear() {
	for (int i = 0; i < com_graph_hash_list_length; ++i) {
		com_hash_table[i].clear();
	}
}
/*void com_graph_hash::write_back_to_vector(vector<int> edge_of_com_graph[SOC_NUM], vector<int> weight_of_edge_of_com_graph[SOC_NUM]) {//鎶婂搱甯岃〃涓�鐨勫唴瀹瑰啓鍥炲埌杩欏叏灞€鍙橀噺涓�
	for (int i = 0; i < com_graph_hash_list_length; ++i) {
		for (vector<com_graph_neighbour>::iterator it = com_hash_table[i].begin(); it < com_hash_table[i].end(); ++it) {

		}
	}
}*/
//判断一个节点是否属于某个社团
//使用的全局：community_each_node_in
bool is_in_this_community(int node, int com) {
	for (vector<int>::iterator it = community_each_node_in[node].begin(); it < community_each_node_in[node].end(); ++it) {
		if (*it == com) {
			return true;
		}
	}
	return false;
}

//找出两个节点共同在的社团
//使用的全局：community_each_node_in
vector<int> find_common_community(int node1, int node2) {
	vector<int> common_community;
	for (vector<int>::iterator it1 = community_each_node_in[node1].begin(); it1 < community_each_node_in[node1].end(); ++it1) {
		for (vector<int>::iterator it2 = community_each_node_in[node2].begin(); it2 < community_each_node_in[node2].end(); ++it2) {
			if (*it1 == *it2) {
				common_community.push_back(*it1);
			}
		}
	}
	return common_community;
}

//构建社团图,社团图用来判断网络的连通性，目的是降低判断连通性的复杂度，但是好像为此折腾了好多东西
//使用的全局变量：SOC_NUM,group,edge,community_each_node_in修改的全局变量：edge_of_com_graph,weight_of_edge_of_com_graph,com_nodes
void build_com_graph() {
	for (int i = 0; i < SOC_NUM; ++i) {//遍历所有社团
		//cout <<"tracking " <<i << endl;
		for (vector<int>::iterator group_it = group[i].begin(); group_it < group[i].end();++group_it) {//遍历社团i中所有节点
			//cout << *group_it << endl;
			for (vector<int>::iterator node_it = edge[*group_it].begin(); node_it < edge[*group_it].end(); ++node_it) {//遍历节点中的所有边
				if (!is_in_this_community(*node_it,i)) {//节点不在当前社团，则把当前社团和该节点所在的所有社团连接起来
					if (is_in_one_community(*node_it, *group_it)) {
					//如果实际上这两个节点是在一个社团j内的，说明有一个节点是同属i,j两个社团，在遍历j社团时不会将这个边识别成社团间的边，于是会出现i的邻接表有j，j的邻接表没有i的情况，因此这里需要处理一下
						vector<int> common_community(find_common_community(*node_it, *group_it));
						for (vector<int>::iterator it = common_community.begin(); it < common_community.end(); ++it) {
							int index = com_nodes[*it].find_index(i);
							if (index == -1) {
								edge_of_com_graph[*it].push_back(i);
								weight_of_edge_of_com_graph[*it].push_back(1);
								com_nodes[*it].insert_neighbor(i, edge_of_com_graph[*it].size() - 1);
							}
							else {
								weight_of_edge_of_com_graph[*it][index]++;
							}
						}
					}
					for (vector<int>::iterator com_it = community_each_node_in[*node_it].begin(); com_it < community_each_node_in[*node_it].end(); com_it++) {//遍历邻接的节点所在的所有社团，每个社团都往当前社团连一条边
						int index = com_nodes[i].find_index(*com_it);
						if (index == -1) {
							//cout << *com_it << endl;
							edge_of_com_graph[i].push_back(*com_it);
							weight_of_edge_of_com_graph[i].push_back(1);
							com_nodes[i].insert_neighbor(*com_it, edge_of_com_graph[i].size() - 1);
						}
						else {
							weight_of_edge_of_com_graph[i][index]++;
						}
					}
				}
			}
		}
	}
}


//输出社团图邻接表（测试用）
void display_com_graph() {
	cout <<"edges:"<< num_of_edges << endl;
	int cnt = 0;
	for (int i = 0; i < SOC_NUM; ++i) {
		cout << "community " << i << ":";
		for (vector<int>::iterator it = edge_of_com_graph[i].begin(); it < edge_of_com_graph[i].end(); ++it) {
			cout << *it << " ";
		}
		cout << endl << "weight:      ";
		for (vector<int>::iterator it = weight_of_edge_of_com_graph[i].begin(); it < weight_of_edge_of_com_graph[i].end(); ++it) {
			cout << *it << " ";
			cnt += *it;
		}
		cout << endl;
	}
	cout << "outside edge:" << cnt/2 << endl;
	cout << "oustside edge size:" << edge_outside_community.size() << endl;
}

//取社团外的边
//使用的全局变量：num_of_edges，all_edges,修改的全局变量：edge_outside_community
void get_edge_outside_community() {
	for (int i = 0; i < num_of_edges; ++i) {
		if (!is_in_one_community(all_edges[i].a, all_edges[i].b)) {
			edge_outside_community.push_back(all_edges[i]);
		}
	}
}

//利用社团图判断网络的连通性,用广搜
//使用的全局变量：edge_of_com_graph
bool is_connected() {
	queue<int> com_bfs;
	int cnt = 0;
	com_bfs.push(0);
	bool* tracked = new bool[SOC_NUM];
	for (int i = 0; i < SOC_NUM; ++i) {
		tracked[i] = 0;
	}
	tracked[0] = 1;
	while (com_bfs.size() != 0) {
		int cur = com_bfs.front();
		com_bfs.pop();		
		cnt++;
		for (vector<int>::iterator it = edge_of_com_graph[cur].begin(); it < edge_of_com_graph[cur].end(); it++) {
			if ((!tracked[*it])&& *it != -1) {
				com_bfs.push(*it);
				tracked[*it] = 1;
			}
		}
	}
	delete []tracked;
	return (cnt == SOC_NUM);
}

//直接全网络广搜判断连通性
bool is_connected2() {
	queue<int> bfs;
	int cnt = 0;
	bfs.push(0);
	bool* tracked = new bool[NODE_NUM];
	for (int i = 0; i < NODE_NUM; ++i) {
		tracked[i] = 0;
	}
	tracked[0] = 1;
	while (bfs.size() != 0) {
		int cur = bfs.front();
		bfs.pop();
		cnt++;
		for (vector<int>::iterator it = edge[cur].begin(); it < edge[cur].end(); it++) {
			if ((!tracked[*it]) && *it != -1) {
				bfs.push(*it);
				tracked[*it] = 1;
			}
		}
	}
	delete []tracked;
	return (cnt == NODE_NUM);
}

//
void guarantee_network_connected(){
	cout<<"Guarantee that the network is connected...\n";
	vector<int> connected;
	while(1){
		queue<int> bfs;
		bfs.push(0);
		connected.push_back(0);
		int cnt = 0;
		bool* tracked = new bool[NODE_NUM];
		for (int i = 0; i < NODE_NUM; ++i) {
			tracked[i] = 0;
		}
		tracked[0] = 1;
		int cur;
		
		while (bfs.size() != 0) {
			int cur = bfs.front();
			bfs.pop();
			cnt++;
			for (vector<int>::iterator it = edge[cur].begin(); it < edge[cur].end(); it++) {
				if ((!tracked[*it]) && *it != -1) {
					bfs.push(*it);
					tracked[*it] = 1;
					connected.push_back(*it);
				}
			}
		}
		if(cnt == NODE_NUM){
			delete []tracked;
			break;
		}
		for(int i = 0;i < NODE_NUM;++i){
			if(!tracked[i]){
				link_one_edge(i,connected[rand()%connected.size()]);
				connected.clear();
				break;
			}
		}
		delete []tracked;
	}
	
}
//检查每个社团内部的连通性，（用SCP划分得到的社团可以保证连通，不需要检查，随机网络的需要），如果不连通时通过随机加边使之连通
void guarantee_community_is_connected() {
	cout << "checking the connectivity of communities...\n";
	for (int i = 0; i < SOC_NUM; ++i) {
		cout << "\r     \r" << (i + 1) * 100 / SOC_NUM <<"%";
		while(1){
			queue<int> bfs;//广搜队列
			bfs.push(group[i][0]);
			bool* visit = new bool[group[i].size()];
			visit[0] = 1;
			for (int j = 1; j < group[i].size(); ++j) {
				visit[j] = 0;
			}
			//广搜
			while (!bfs.empty()) {
				int cur = bfs.front();
				for (vector<int>::iterator it = edge[cur].begin(); it < edge[cur].end(); ++it) {
					for (int j = 0; j < group[i].size(); j++) {
						if (group[i][j] == *it) {
							if (visit[j] == 0) {
								visit[j] = 1;
								bfs.push(*it);
							}
							break;
						}
					}
				}
				bfs.pop();
			}
			bool connect = true;
			//检查节点有没有都被遍历到
			for (int j = 0; j < group[i].size(); ++j) {
				if (!visit[j]) {//没有被遍历到则随机在社团中选一个被遍历到的点连接 
					connect = false;
					int rand_link = rand() % group[i].size();
					while (!visit[rand_link]) {//循环找到一个被遍历过的为止
						rand_link++;
						rand_link %= group[i].size();
					}
					//加边
					visit[j] = 1;
					link_one_edge(group[i][j], group[i][rand_link]);
					break;
				}
			}
			delete[]visit;
			if(connect) break;
		}
		
	}
}
//随机地把数目为num_of_edges_to_change的社团外的边重连到社团内，以加强社团强度
//使用的全局变量： 修改的全局变量：edge_outside_community(因为是vector类型，所以为了效率，用把两条边赋值成-1来代表删去，完成重连后再用get_edge_outside_community弄好)
//								   weight_of_edge_of_com_graph,edge_of_com_graph,edge,all_edges
void reconnect(int num_of_edges_to_change) {
	cout << "reconnecting edges...\n";
	Node cut_edge;//借用Node的哈希表，存放割边的编号
	cut_edge.setDegree(SOC_NUM/2);
	int cut_edge_cnt = 0;//割边的数目
	int edge_outside_com_cnt = edge_outside_community.size();//当前还剩下的社团间的边
		for (int i = 0; i < num_of_edges_to_change; ++i) {
		
		cout << "\r     \r" << (i * 100+100) / num_of_edges_to_change << "%";
		
		if (cut_edge_cnt == edge_outside_com_cnt) {//所有边都是割边
			cout << "\n已无法再删去任何社团间的边！\n";
			break;
		}
		/*if (!is_connected()) {
			cout << "缃戠粶涓嶈繛閫氾紒\n";

		}*/
		int rand_num = ((rand() / 2 << 16) | rand()) % 10000;//获得一个随机数
		//rand_num *= 1000;
		rand_num %= edge_outside_community.size();
		int rand_num_change_cnt = 0;//随机数改变次数
		while (cut_edge.is_neighbor(rand_num)||(edge_outside_community.at(rand_num).a==-1)) {//如果是割边,或已去除的边，则重新找，这可能导致社团间的边不多时要找很久//at()有对元素是否越界进行判断
			rand_num_change_cnt++;
			rand_num++;//改变随机数
			rand_num%=edge_outside_community.size();
			if(rand_num_change_cnt>=edge_outside_community.size()){
				cout << "\n已无法再删去任何社团间的边！\n";
				break;
			}
		}
		if(rand_num_change_cnt>=edge_outside_community.size()) break;//已无法再删去任何社团间的边
		int node1 = edge_outside_community.at(rand_num).a;
		int node2 = edge_outside_community.at(rand_num).b;
		bool connected = 1;//网络是否还连通
		//遍历两个节点所在的所有社团，逐一把它们之间的权重减一，如果权重减到0，则把邻接表中对应位置改成-1
		vector<int>::iterator it1, it2;
		for (it1 = community_each_node_in[node1].begin(); it1 < community_each_node_in[node1].end(); ++it1) {
			//cout <<"checking "<< *it1 << endl;
			for (it2 = community_each_node_in[node2].begin(); it2 < community_each_node_in[node2].end(); ++it2) {
				int index1 = com_nodes[*it1].find_index(*it2);//获得*it2在*it1的邻接表中的下标
				int index2 = com_nodes[*it2].find_index(*it1);//获得*it1在*it2的邻接表中的下标
				weight_of_edge_of_com_graph[*it1].at(index1)--;//权重减一
				weight_of_edge_of_com_graph[*it2].at(index2)--;
				if (weight_of_edge_of_com_graph[*it1].at(index1) == 0) {//权重减到0
					edge_of_com_graph[*it1].at(index1) = -1;//邻接表对应位置赋值为-1
					edge_of_com_graph[*it2].at(index2) = -1;
					connected = is_connected();
					if (!connected) {//如果不连通了，说明该边为割边不能去掉，并记录下来，需要再次查找
						cut_edge.insert_neighbor(rand_num);//记录该边
						cut_edge_cnt++;
						break;
					}
				}
			}
			if (!connected) break;
		}

		if (connected) {//去边成功，重连一条边
			//cout << "reconnect successfully\n";
			
			//重连的边在社团内，因此和社团图没有太大关系了

			int rand_node_choose;
			int rand_num2 = rand() % 100000;

			if (rand_num2 % 2) {//随机选边的一个端点
				rand_node_choose = node1;
			}
			else {
				rand_node_choose = node2;
			}

			int rand_com_choose_index = rand_num2 % community_each_node_in[rand_node_choose].size();//随机选该端点所在的一个社团
			int rand_com_choose = community_each_node_in[rand_node_choose][rand_com_choose_index];

			Node rand_node(&edge[rand_node_choose][0], edge[rand_node_choose].size(),0);//放到哈希表去，方便查找
			int loop_cnt = 0;//计算循环次数，如果太多说明可能这个社团里面所有顶点和该顶点都有边
			int all_connected_group_cnt = 0;//节点所在的社团中，所有点都和该点相连的社团数
			while (1) {
				loop_cnt++;
				//cout << loop_cnt << endl;
				if (loop_cnt % COMMUNITY_SIZE==0) {//循环次数=社团大小
					
					bool not_connect = 0;//是否有未和当前节点连接的节点
					for (vector<int>::iterator it = group[rand_com_choose].begin(); it < group[rand_com_choose].end(); ++it) {//遍历社团中的所有节点
						if ((!rand_node.is_neighbor(*it))&&(*it!=rand_node_choose)) {//有未和当前节点连接的节点
							not_connect = 1;
							break;
						}
					}
					if (!not_connect) {//没有未和当前节点连接的节点
						all_connected_group_cnt++;//所有点都和该点相连的社团数++
						if (all_connected_group_cnt == community_each_node_in[rand_node_choose].size()) {//该节点所有社团中的所有点都和该点相连
							it1--;
							it2--;
							//connected = 0;//connected置0，然后等同于去边失败来处理
							all_connected_group_cnt = 0;
							break;
						}
						rand_com_choose_index = (rand_com_choose_index+1)% community_each_node_in[rand_node_choose].size();//出现有所有节点都和当前节点连接的社团，则把选中的社团往下移一个
						rand_com_choose = community_each_node_in[rand_node_choose][rand_com_choose_index];
					}
				}
				rand_num2 = ((rand() / 2 << 16) | rand()) % 100000;
				int rand_node_to_connect_choose = group[rand_com_choose][rand_num2 % group[rand_com_choose].size()];//随机选该社团中的一个点
				if ((!rand_node.is_neighbor(rand_node_to_connect_choose))&&(rand_node_to_connect_choose != rand_node_choose)) {//如果不是邻居且不是自己，则连接
					edge[rand_node_choose].push_back(rand_node_to_connect_choose);
					edge[rand_node_to_connect_choose].push_back(rand_node_choose);
					Edge new_edge;
					new_edge.a = rand_node_choose;
					new_edge.b = rand_node_to_connect_choose;
					all_edges.push_back(new_edge);
					all_connected_group_cnt = 0;
					break;
				}
			}

			if (connected) {
				//维护edge_outside_community
				edge_outside_community[rand_num].a = -1;
				edge_outside_community[rand_num].b = -1;
				edge_outside_com_cnt--;

				//在edge中去掉一条边,暂时没实现维护all_edge
				for (vector<int>::iterator node1_it = edge[node1].begin(); node1_it < edge[node1].end(); ++node1_it) {//节点1的边中去除一个节点2
					if (*node1_it == node2) {
						edge[node1].erase(node1_it);
						break;
					}
				}
				for (vector<int>::iterator node2_it = edge[node2].begin(); node2_it < edge[node2].end(); ++node2_it) {//节点2的边中去除一个节点1
					if (*node2_it == node1) {
						edge[node2].erase(node2_it);
						break;
					}
				}
			}
			
		}
		if(!connected){//去边失败，恢复修改的值,不用else因为在上一个if里，如果重连时发现已经连接到了所有的社团内的顶点，会把connected置0
			//cout << "reconnect fail\n";
			//cout << cut_edge_cnt << endl;
			vector<int>::iterator itt1, itt2;
			for (itt1 = community_each_node_in[node1].begin(); itt1 <= it1&&itt1 < community_each_node_in[node1].end() ; ++itt1) {

				vector<int>::iterator end_;
				if (itt1 == it1) {
					end_ = it2+1;
				}
				else {
					end_ = community_each_node_in[node2].end();
				}

				for (itt2 = community_each_node_in[node2].begin(); itt2 < end_; ++itt2) {
					int index1 = com_nodes[*itt1].find_index(*itt2);//获得*itt2在*itt1的邻接表中的下标
					int index2 = com_nodes[*itt2].find_index(*itt1);//获得*itt1在*itt2的邻接表中的下标
					weight_of_edge_of_com_graph[*itt1].at(index1)++;//权重加一
					weight_of_edge_of_com_graph[*itt2].at(index2)++;
					if (weight_of_edge_of_com_graph[*it1].at(index1) == 1) {//权重原本已经减到0
						edge_of_com_graph[*itt1].at(index1) = *itt2;//邻接表对应位置重新赋值为对应的邻居
						edge_of_com_graph[*itt2].at(index2) = *itt1;
						
					}
				}
			}
			connected = 1;
			i--;
		}
	}
	cout << "\nreconnect finish\n";
//完成所有边的重连，维护修改的全局变量
//维护edge_outside_community
	vector<Edge>::iterator it2 = edge_outside_community.end()-1;
	for (vector<Edge>::iterator it1 = edge_outside_community.begin(); it1 <= it2; ++it1) {//閬嶅巻鍒拌�佽��鍒犲幓鐨勫厓绱犳椂锛屽彇鏈€鍚庣殑涓€涓�鍏冪礌琛ュ洖
		if (it1->a == -1) {
			while (it2->a == -1 && it2 > it1) {
				it2--;
			}
			if (it2 <= it1) {
				edge_outside_community.erase(it2, edge_outside_community.end());
				break;
			}
			*it1 = *it2;
			it2--;
		}
		if (it2 <= it1) {
			edge_outside_community.erase(it2+1, edge_outside_community.end());
			break;
		}
	}
	//维护weight_of_edge_of_com_graph,edge_of_com_graph,edge
	for (int i = 0; i < SOC_NUM; ++i) {
		vector<int>::iterator it2 = edge_of_com_graph[i].end()-1;
		for (vector<int>::iterator it1 = edge_of_com_graph[i].begin(); it1 <= it2; ++it1) {//遍历到要被删去的元素时，取最后的一个元素补回
			if (*it1 == -1) {
				while (*it2 == -1&&it2 > it1) {
					it2--;
				}
				if (it2 <= it1) {
					edge_of_com_graph[i].erase(it2, edge_of_com_graph[i].end());
					break;
				}
				*it1 = *it2;
				it2--;
			}
			if (it2 <= it1) {
				edge_of_com_graph[i].erase(it2+1, edge_of_com_graph[i].end());
				break;
			}
		}
		it2 = weight_of_edge_of_com_graph[i].end()-1;
		for (vector<int>::iterator it1 = weight_of_edge_of_com_graph[i].begin(); it1 <= it2; ++it1) {//遍历到要被删去的元素时，取最后的一个元素补回
			if (*it1 == 0) {
				while (*it2 == 0 && it2 > it1) {
					it2--;
				}
				if (it2 <= it1) {
					weight_of_edge_of_com_graph[i].erase(it2, weight_of_edge_of_com_graph[i].end());
					break;
				}
				*it1 = *it2;
				it2--;
			}
			if (it2 <= it1) {
				weight_of_edge_of_com_graph[i].erase(it2+1, weight_of_edge_of_com_graph[i].end());
				break;
			}
		}
	}
	//维护com_nodes
	for (int i = 0; i < SOC_NUM; ++i) {
		com_nodes[i].clear();
		for (int j = 0; j < edge_of_com_graph[i].size(); ++j) {
			com_nodes[i].insert_neighbor(edge_of_com_graph[i][j], j);
		}
	}
	//all_edges的维护成本有点高，暂时没想到好的维护方法
}


//=========================================================================================
//CDN算法(by梁萍佳)
//=========================================================================================

void CDN() {
	static int first_time=0;
	if(first_time==0){
		zipf_pdf(z, popularity);
		int i = 0;
		for (int i = 0; i < NODE_NUM; i++) {
			for (int t = 0; t < CACHE_SIZE; t++) {
				file[i].push_back(FileCache(-1,0));//鍒濆�嬬紦瀛�
			}
		}		
	}
	first_time=1;
	for (int i = 0; i < FILE_NUM; i++)source[i] = rand() % NODE_NUM;//鏂囦欢闅忔満鏀� 绗琲涓�鏂囦欢鏀惧湪 source[i]涓�鑺傜偣

																	//----------涓嬮潰杩欓儴鍒嗗氨鏄�澶勭悊涓€涓嬭緭鍑鸿€屽凡锛屼笉鐢ㄥ湪鎰�------------------
	char buf[1000];
	sprintf(buf, "file_num:%d node_num:%d demand_num:%d community_size:%d\nin_pro:%lf,out_link:%d,cache_size:%d\n",
		FILE_NUM, NODE_NUM, DEMAND_NUM, COMMUNITY_SIZE, IN_PRO, OUT_LINK, CACHE_SIZE);
	//	fprintf(ofile,"%s", buf);
	cout << buf << endl;
	//------------寮€濮嬩簡杩愯�屼簡--------------LCE绗�涓€涓�
	double sum_time;
	hit_times = copy_times = replace_times=0;
	cout << "====================================================" << endl;
	printf("LCE use time = %lf\n", sum_time=object1(popularity, edge, file, LCE));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"LCE use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\t",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
		//fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();
	
	cout << "====================================================" << endl;
	printf("LCD use time  = %lf\n", sum_time=object1(popularity, edge, file, LCD));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"LCD use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\t",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
		//fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();
	
	cout << "====================================================" << endl;
	printf("SOC use time  = %lf\n", sum_time=object1(popularity, edge, file, SOC));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"SOC use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\t",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
	//	fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();
	
	cout << "====================================================" << endl;
	printf("SOC2 use time  = %lf\n", sum_time=object1(popularity, edge, file, SOC2));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"SOC use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\t",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
	//	fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();	
	
	cout << "====================================================" << endl;
	printf("SOCD use time  = %lf\n", sum_time=object1(popularity, edge, file, SOCD));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"SOCD use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\t",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
		//fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();

	cout << "====================================================" << endl;
	printf("SOCD2 use time  = %lf\n", sum_time=object1(popularity, edge, file, SOCD2));
	printf("Average response time = %lf\n",sum_time/DEMAND_NUM);
	printf("replace times=%d ",replace_times);
	printf("hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	//fprintf(result,"SOCD use time = %lf\n", sum_time);
	//fprintf(result,"Average response time = %lf\n",sum_time/DEMAND_NUM);
	//fprintf(result,"hit times:%d,hit rate:%lf ,copy times=%d\n", hit_times, (double)hit_times / DEMAND_NUM,copy_times);
	fprintf(result,"%lf\t%lf\t%d\t%d\n",(double)hit_times / DEMAND_NUM,sum_time/DEMAND_NUM,copy_times,replace_times);
	for(int i=0;i<10;i++){
		printf("number of demand that use %d time:%d\n",i,time_count[i]);
		//fprintf(result,"number of demand that use %d time:%d\n",i,time_count[i]);
	}
	cout << "====================================================" << endl;
	reinitialize();
}
void reinitialize(){
	for (int i = 0; i < NODE_NUM; i++) {//清空一下缓存，准备下一个
		time_count[i]=0;
		for (int t = 0; t < file[i].size(); t++) {
			file[i][t] = -1;
		}
	}
	hit_times = copy_times = replace_times=0;
}

double drand() {
	return fabs((rand() << 16 | rand()) % 10000000 / 10000000.0);
}
void refresh(int node, int fnum,int time) {//file[node][fnum] is used, so this function will reset its TTL, by put it in file[node][0]
	int fn = file[node][fnum].filenum;
	for (int i = fnum; i > 0; i--) {
		file[node][i] = file[node][i - 1];
	}
	file[node][0] = FileCache(fn,time);

}

void replace1(int n, int f,int time) {//put the file f into the cache of node n,and this will replace the oldest cache
	if(file[n][file[n].size()-1].filenum!=-1)replace_times++;
	for (int k = file[n].size() - 1; k > 0; k--) {
		file[n][k] = file[n][k - 1];
	}
	file[n][0] = FileCache(f,time);
	copy_times++;
}
int rand_file() {//闅忔満鐢熸垚涓€涓�瑕佽�块棶鐨勬枃浠讹紝渚濇�傜巼
	double pro;
	int demand_file, bisec_left, bisec_right;
	pro = drand();//random file according to popularity //闅忔満浜х敓涓€涓�鏁帮紝鐪嬭惤鍦ㄥ摢涓�鏂囦欢鐨勬�傜巼鑼冨洿鍐�
	demand_file = FILE_NUM / 2;
	bisec_left = 0;
	bisec_right = FILE_NUM; //杩欓噷瑕�-1锛�
	while (1) {//鎵惧嚭钀界偣pro鍦ㄥ摢涓€涓猍  add[k],add[k+1] )涔嬮棿锛岄偅杩欎釜k灏辨槸瑕佹壘鐨刣emand_file锛屽乏闂�鍙冲紑
		if (pro >= addup[demand_file + 1] && demand_file < FILE_NUM) {
			bisec_left = demand_file;
			demand_file = (demand_file + bisec_right) / 2;
			if (demand_file == bisec_left)break;//鐜板湪demand_file灏辩瓑浜巄isec_right-1浜嗭紝涔熷氨鏄痜ilenum-1
			continue;
		}
		if (pro < addup[demand_file]) {
			bisec_right = demand_file;
			demand_file = (demand_file + bisec_left) / 2;
			continue;
		}
		break;//addup[f]<=pro<addup[f+1]
	}
	return demand_file;
}
struct Qnode {//骞挎悳鏍戠敤鐨勭粨鐐�
	int node;//缁撶偣搴忓彿
	int level;//缁撶偣娣卞害
	Qnode(int n = 0, int l = 0) {
		node = n;
		level = l;
	}
};
double object1(double popularity[], vector<int>edge[], vector<FileCache>file[], CACHE_STRATEGY strategy) {
	int i, j, k, demand_node, demand_file, bisec_left, bisec_right, hit;
	//whether it have been access
	static vector<int> visited_father[MAX_NODE_NUM];//the father node in bfs tree
	int  level;//the current length beteen the node and the bfs tree root
	static queue<Qnode> que;
	Qnode qnode;
	static int dep[MAX_NODE_NUM];//dep of the node in bfs tree
	double count = 0, pro;//count is used to record the sum of the lengh in each demand
	
	for (i = 0; i < DEMAND_NUM; i++) {//each loop represents a demand
		//------------------generate a random node and a random file number to simulate a demand-----
		if (i % DEMANDSHOW == 0)printf("demand %d\n", i);
		demand_node = ((rand() / 2 << 16) | rand()) % NODE_NUM;//random node 
		demand_file = rand_file();
		//--------------------------------------------------------------------------------------------

		//-------------------------------------start BFS----------------------------------------------
		hit = 0;
		for (j = 0; j < NODE_NUM; j++) {
			visited_father[j].clear();
		}
		for (j = 0; j < NODE_NUM; j++) {
			dep[j] = 0x7ffffff;
		}
		while (!que.empty())que.pop();
		que.push(Qnode(demand_node, 0));//push the root node (the node that demand a file) in the que
		visited_father[demand_node].push_back(demand_node);//
		int quit = 0;//
		static vector<Qnode> result;//if there are multiple choice,randomly chose one
		result.clear();
		level=1000;//just a big number..
		// level is used to record length between  the demand node and the closest node that has file
		while (!que.empty()) {
			qnode = que.front();
			que.pop();
			if(level<qnode.level)break;//the node is deeper than the result node,so we can stop here
			//----find out if the file is in this node----
			if (source[demand_file] == qnode.node) {//it is the source?
				result.push_back(qnode);
				level = qnode.level;//record the length
				quit = 1;
			}
			for (k = 0; k < file[qnode.node].size(); k++) {//or does it cache the file?
				if (file[qnode.node][k].filenum == demand_file) {
					result.push_back(qnode);
					refresh(qnode.node, k,i);//refresh the cache,because it hits
					level = qnode.level;
					quit = 1;
				}
			}
			if (quit == 1)continue;//now it already find at least one node that has the file,so dont need deeper
			//-------------------------------------
			for (k = 0; k < edge[qnode.node].size(); k++) {//put its neighbor in the queue
				if (dep[edge[qnode.node][k]] == qnode.level + 1) {//this node is put into the queue by another node
				//which the depth is the same,so this node has one more father
					visited_father[edge[qnode.node][k]].push_back(qnode.node);
				}
				if (dep[edge[qnode.node][k]] > qnode.level + 1) {//this node has not been put into the queue
					dep[edge[qnode.node][k]] = qnode.level + 1;
					que.push(Qnode(edge[qnode.node][k], qnode.level + 1));
					visited_father[edge[qnode.node][k]].push_back(qnode.node);
				}

			}
		}
		//--------------------------------------------------------------------------------

		//------------------------now BFS is finished,we can start  caching---------------
		//void replace(node,file,time),i is the time stamp
		switch (strategy) {
		case LCE: {

			int ran = rand() % result.size();
			k = result[ran].node;//randomly chose one node 

			if (source[demand_file] == k) {//it is the source
				hit_times++;//the hit_times will be reverse in the end and represents the times it hit a cache
				//now it records the times it miss
			}

			while (k != demand_node) {//find the route back to the demand node
				replace1(k, demand_file,i);//cache the file
				ran = rand() % visited_father[k].size();//randomly find a father node
				k = visited_father[k][ran];
			}
			replace1(demand_node, demand_file,i);
			break;
		}
		case LCD: {
			int ran = rand() % result.size();
			k = result[ran].node;

			if (source[demand_file] == k) {
				hit_times++;
			}

			while (visited_father[k][0] != demand_node) {//find the route back to the demand node
				ran = rand() % visited_father[k].size();
				k = visited_father[k][ran];
			}
			replace1(k, demand_file,i);//cache in the closest node
			break;
		}
		case SOC: {
			static bool mark[MAX_SOC_NUM];//mark[i] is true only when the social group i has cache the file
			memset(mark, 0, sizeof(mark));
			int ran = rand() % result.size();
			k = result[ran].node;//random chose 
			
			mark[community_each_node_in[k][0]] = true;//this node already has the file,so is its group
			if (source[demand_file] == k) {//miss
				hit_times++;//it will reverse
			}

			while (k != demand_node) {//find the way back to the demand node
			
				int idle_node=k;
				if (!mark[community_each_node_in[k][0]]) {//if this group has not been marked to have the file
					for (int t1 = 0; t1 < group[community_each_node_in[k][0]].size(); t1++) {//for each node in the group
						for (int t2 = 0; t2 < file[group[community_each_node_in[k][0]][t1]].size(); t2++) {//for each file in the node
							if (file[group[community_each_node_in[k][0]][t1]][t2].filenum == demand_file) {//if this cache is the file
								refresh(group[community_each_node_in[k][0]][t1],t2,i); //refresh it
								goto b1;//break the loop,and don't need to cache
							}
							if (file[group[community_each_node_in[k][0]][t1]][t2].time < file[idle_node][file[idle_node].size()-1].time) {
								//if this node is not full,record it
								idle_node=group[community_each_node_in[k][0]][t1];
							}							
						}
					}
					replace1(idle_node, demand_file,i);//no one node has the file,then cache it in the node
				}
				b1:
				mark[community_each_node_in[k][0]] = true;
				//------------finish caching now,continue to find the way back-----------
				ran = rand() % visited_father[k].size();//闅忔満鎸戜釜鐖惰妭鐐�
				k = visited_father[k][ran];
			}
			if(!mark[community_each_node_in[demand_node][0]])replace1(demand_node, demand_file,i);
			//if the demand node's group still does not has the file,then cache it
			break;

		}
		case SOC2: {//not total soc
			static bool mark[MAX_SOC_NUM];//mark[i] is true only when the social group i has cache the file
			memset(mark, 0, sizeof(mark));
			int ran = rand() % result.size();
			k = result[ran].node;//random chose 
			
			mark[community_each_node_in[k][0]] = true;//this node already has the file,so is its group
			if (source[demand_file] == k) {//miss
				hit_times++;//it will reverse
			}

			while (k != demand_node) {//find the way back to the demand node
			
				int idle_node=k;
				if (!mark[community_each_node_in[k][0]]) {//if this group has not been marked to have the file
					replace1(idle_node, demand_file,i);//just cache it in the node
				}
				mark[community_each_node_in[k][0]] = true;
				//------------finish caching now,continue to find the way back-----------
				ran = rand() % visited_father[k].size();//
				k = visited_father[k][ran];
			}
			if(!mark[community_each_node_in[demand_node][0]])replace1(demand_node, demand_file,i);
			//if the demand node's group still does not has the file,then cache it
			break;

		}
		case SOCD: {
			int ran = rand() % result.size();
			k = result[ran].node;//闅忔満鎸戜釜鑺傜偣

			if (source[demand_file] == k) {//灏辨槸婧愭湇鍔″櫒浜�
				hit_times++;//寰呬細浼氬弽杞�鐨勶紝鐜板湪鍏堣�板綍miss鐨勬�℃暟
				if (SHOW)cout << endl << "not hit at cache.";
			}
			if (ROUTE) { cout << endl << "route: "; }

			while (visited_father[k][0] != demand_node) {//涓€璺�寰€涓婃壘鍒板彂鍑鸿�锋眰鐐圭殑瀛愯妭鐐�
				if (ROUTE)	cout << " " << k;
				ran = rand() % visited_father[k].size();
				k = visited_father[k][ran];
			}
			int group_num=community_each_node_in[k][0],mark=0,idle_node=k;
			for(int k=0;k<group[group_num].size();k++){
				for(int j=0;j<file[group[group_num][k]].size();j++){
					if(file[group[group_num][k]][j].filenum==demand_node){
						refresh(group[group_num][k],j,i);
						mark=1;
						break;
					}
					if(file[group[group_num][k]][j].time<file[idle_node][file[idle_node].size()-1].time){
						idle_node=group[group_num][k];
					}					
				}
				if(mark==1)break;
			}
			if(mark==0)replace1(idle_node, demand_file,i);
			break;
		}
		case SOCD2: {//not tatal socd
			int ran = rand() % result.size();
			k = result[ran].node;//

			if (source[demand_file] == k) {//
				hit_times++;//
			}

			while (visited_father[k][0] != demand_node) {//if the father is not the demand node,then find the way back
				ran = rand() % visited_father[k].size();
				k = visited_father[k][ran];
			}
			int group_num=community_each_node_in[k][0],mark=0;
			for(int l=0;l<group[group_num].size();l++){//for each member in the group
				for(int j=0;j<file[group[group_num][l]].size();j++){//for each file cached in this member
					if(file[group[group_num][l]][j].filenum==demand_node){//if it has the file,then no need to cache
						refresh(group[group_num][l],j,i);
						mark=1;
						break;
					}		
				}
				if(mark==1)break;
			}
			if(mark==0)replace1(k, demand_file,i);
			break;
		}
		default:
			break;
		}
		//------------------------涓婇潰閮ㄥ垎鏍规嵁骞挎悳缁撴灉杩涜�岀紦瀛�----------------
		count += level;//杩欓噷鍔犱竴涓嬪箍鎼滄爲娣卞害鐨勮�板綍
		time_count[level]++;
					   //鐒跺悗缁х画杩涜�屼笅涓€涓�璇锋眰鐨勫�勭悊鍟�
	}
	hit_times = DEMAND_NUM - hit_times;//缈昏浆涓€涓嬶紝鍥犱负鍘熸潵璁扮殑鏄�锛宧it鍒皊ource鐨�
	return count;
}

void zipf_pdf(double z, double lambda[]) {		//zipf file popularity distribution with shape parameter z
	double c = 0;					//cummulative
	int k;						//rank
	for (k = 1; k < FILE_NUM + 1; k++) {
		c = c + 1.0 / pow(k, z);
	}
	for (k = 0; k < FILE_NUM; k++) {
		lambda[k] = 1.0 / pow(k + 1, z) / c;
	}
	for (k = 1; k <= FILE_NUM; k++) {
		addup[k] = addup[k - 1] + lambda[k - 1];//addup[k]=灏忎簬k鐨勬枃浠跺簭鍙风殑闇€姹傚害涔嬪拰
												//	cout << addup[k] << " ";
	}
	//randomly permute the rank of the files in every leaf node

}


//清空所有的边(edge,num_of_edges,all_edges)
void clear_edges() {
	for (int i = 0; i < NODE_NUM; ++i) {
		edge[i].clear();
	}
	num_of_edges = 0;
	all_edges.clear();
}

void clear_all(){
	clear_edges();
	for(int i = 0;i < SOC_NUM;++i){
		group[i].clear();
		com_nodes[i].clear();
		edge_of_com_graph[i].clear();
		weight_of_edge_of_com_graph[i].clear();
	}
	for(int i = 0;i < NODE_NUM;++i){
		community_each_node_in[i].clear();
	}
	edge_outside_community.clear();
} 


//Node类，表示一个节点，用哈希表存储它的邻居 
Node::Node() {
	node_name = 0;
	_degree = 0;
	neighbor_hash_table = NULL;
}
Node::Node(const Node& other) {
	this->_degree = other._degree;
	int hash_table_size = _degree / offset + 1;
	neighbor_hash_table = new hash_list[hash_table_size];
	for (int i = 0; i < hash_table_size; ++i) {
		neighbor_hash_table[i].assign(other.neighbor_hash_table[i].begin(), other.neighbor_hash_table[i].end());
	}
	node_name = other.node_name;
}
Node::Node(const size_type* array, int size, size_type name) {
	node_name = name;
	_degree = size;
	int hash_table_size = _degree / offset + 1;
	neighbor_hash_table = new hash_list[hash_table_size];

	for (int i = 0; i < size; ++i) {
		neighbor_hash_table[hash(array[i], hash_table_size)].push_back(array[i]);
	}
}
Node::Node(int degree, size_type name) {
	_degree = degree;
	node_name = name;
	neighbor_hash_table = new hash_list[_degree / offset + 1];
}
Node::~Node() {
	if (neighbor_hash_table != NULL) {
		delete[]neighbor_hash_table;
	}
}
void Node::insert_neighbor(const size_type& new_neighbor) {
	int hash_table_size = _degree / offset + 1;
	int index = hash(new_neighbor, hash_table_size);
	assert(index < hash_table_size);

	neighbor_hash_table[index].push_back(new_neighbor);
}
bool Node::is_neighbor(const size_type& target)const {
	int hash_table_size = _degree / offset + 1;
	int index = hash(target, hash_table_size);
	assert(index < hash_table_size);

	for (hash_list::iterator it = neighbor_hash_table[index].begin(); it < neighbor_hash_table[index].end(); ++it) {
		if (*it == target) {
			return true;
		}
	}
	return false;
}
int Node::degree()const {
	return _degree;
}
int Node::hash(const size_type& neighbor, int hash_table_size) {
	int p = 16777619;
	int _hash = (int)2166136261L;
	_hash = (_hash ^ neighbor) * p;
	_hash += _hash << 13;
	_hash ^= _hash >> 7;
	_hash += _hash << 3;
	_hash ^= _hash >> 17;
	_hash += _hash << 5;
	if (_hash < 0) {
		_hash *= -1;
	}
	return _hash % hash_table_size;
}
size_type Node::name()const {
	return node_name;
}
void Node::setName(size_type new_name) {
	node_name = new_name;
}
void Node::setDegree(int new_degree) {
	_degree = new_degree;
	if (neighbor_hash_table != NULL) {
		delete[]neighbor_hash_table;
	}
	neighbor_hash_table = new hash_list[_degree / offset + 1];
}
void Node::test_display()const {
	for (int i = 0; i <= _degree / offset; ++i) {
		cout << "hash list" << i << ": ";
		for (hash_list::iterator it = neighbor_hash_table[i].begin(); it < neighbor_hash_table[i].end(); ++it) {
			cout << *it << " ";
		}
		cout << endl;
	}
}
vector<size_type> Node::common_neighbor(const Node& other)const {
	vector<size_type> common_neighbors;
	common_neighbors.reserve(_degree);


	for (int i = 0; i <= _degree / offset; ++i) {
		for (hash_list::iterator it = neighbor_hash_table[i].begin(); it < neighbor_hash_table[i].end(); ++it) {
			if (other.is_neighbor(*it)) {
				common_neighbors.push_back(*it);
			}
		}
	}
	return common_neighbors;
}
void Node::clear_neighbor() {
	for (int i = 0; i < _degree / offset + 1; ++i) {
		hash_list temp;
		neighbor_hash_table[i].swap(temp);
	}
}

