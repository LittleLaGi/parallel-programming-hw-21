#include "FP_growth.hpp"

#include <pthread.h>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <fstream>
#include <chrono>

#define THREAD_COUNT 4

using namespace std;
using pattern = pair<vector<int>, int>;

/* for pthread */
typedef struct
{
    TreeNode* node;
    unordered_map<int, vector<TreeNode*>> *header;
    pattern prefix;
    unordered_map<int, int> *header_count;
} Arg;
pthread_mutex_t mutex_pat;
vector<map<vector<int>, int>> global_patterns;
int min_support;
int height_constraint = 2;
/* for pthread */

void readData(vector<vector<int>>& data);
void printFpTree(TreeNode* root);
TreeNode* createFpTree(vector<vector<int>>& data, unordered_map<int, vector<TreeNode*>>& header, unordered_map<int, int>& header_count, vector<int>& pattBaseCount);
// TreeNode* createCondFpTree(TreeNode* root, vector<TreeNode*>& head, int min_support);
void findPatterns_main(TreeNode* node, unordered_map<int, vector<TreeNode*>>& header, pattern prefix, unordered_map<int, int>& header_count);
void* findPatterns_thread(void* arg);
void findPatterns_serial(TreeNode* node, unordered_map<int, vector<TreeNode*>>* header, pattern* prefix, map<vector<int>, int>* patterns, unordered_map<int, int>* header_count);
unordered_map<int, int> unordered_items;  // key: item, value: amount of the item
int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "[ERROR] must have two arguments !!\n";
        exit(1);
    }
    min_support = atoi(argv[1]);

    vector<vector<int>> data;
    vector<int> data_count;

    /* read data and count the amount of each item */
    cout << "\n----- [Progress] -----\n";
    cout << "---> read data\n";
    readData(data);
    for (auto& i : data) {
        for (auto item : i) {
            ++unordered_items[item];
        }
    }

    /* remove items with amount smaller than min support */
    cout << "---> preprocess data\n";
    for (auto iter = unordered_items.begin(); iter != unordered_items.end(); ++iter) {
        if (iter->second < min_support)
            unordered_items.erase(iter);
    }

    auto start = std::chrono::high_resolution_clock::now();

    /* build FP tree */
    cout << "---> build FP tree\n";
    unordered_map<int, vector<TreeNode*>> header; // key: item, value: a list of TreeNodes
    unordered_map<int, int> header_count; // key: item, value: sum of the count of every item node
    vector<int> dummy(data.size(), 1);
    TreeNode* root = createFpTree(data, header, header_count, dummy);

    auto end_FpTree = std::chrono::high_resolution_clock::now();

    /* find freq patterns */
    cout << "---> find freq patterns\n\n\n";
    pattern pat;
    map<vector<int>, int> s_patterns;

    findPatterns_main(root, header, pat, header_count);

    auto end_findPattern = std::chrono::high_resolution_clock::now();

    auto final_patterns = global_patterns[0];
    for (auto iter = global_patterns.begin(); iter != global_patterns.end(); ++iter)
        final_patterns.merge(move(*iter));

    /* dump the result */
    cout << "---> print the result\n";
    ofstream outputFile;
    outputFile.open("result.txt");
    for (auto& p : final_patterns) {
        for (auto item : p.first)
            outputFile << item << " ";
        outputFile << ": " << p.second << "\n";
    }
    outputFile.close();

    /* print out timing info */
    std::chrono::duration<double> elapsed1 = end_FpTree - start;
    std::chrono::duration<double> elapsed2 = end_findPattern - end_FpTree;
    cout << '\n';
    cout << "----- [Time] -----\n";
    cout << "total: " << elapsed1.count() + elapsed2.count() << '\n';
    cout << "FpTree: " << elapsed1.count() << '\n';
    cout << "freq pattern: " << elapsed2.count() << '\n';
    cout << "------------------\n\n";

    return 0;
}

void readData(vector<vector<int>>& data) {
    int id = -1, item;
    data.push_back({});
    while (cin >> id >> id >> item) {
        if (id == data.size())
            data.push_back({});
        data[id].push_back(item);
    }
}

TreeNode* createFpTree(vector<vector<int>>& data, unordered_map<int, vector<TreeNode*>>& header,
    unordered_map<int, int>& header_count, vector<int>& pattBaseCount) {

    /* remove items with amount smaller than min support */
    for (size_t i = 0; i < data.size(); ++i) {
        for (auto j : data[i]) {
            auto iter = header_count.find(j);
            if (iter == header_count.end())
                header_count[j] = pattBaseCount[i];
            else
                header_count[j] += pattBaseCount[i];
        }
    }
    for (auto iter = header_count.begin(); iter != header_count.end();) { // remove item in header_count which count < min_support
        if (iter->second < min_support)
            iter = header_count.erase(iter);
        else
            ++iter;
    }

    for (auto& i : data) { // remove the item in data_set which already removed 
        for (auto iter = i.begin(); iter != i.end();) {
            if (header_count.find(*iter) == header_count.end())
                iter = i.erase(iter);
            else
                ++iter;
        }
    }

    /* sort items in each data with their amount in descendent order */
    for (auto& i : data) {
        sort(i.begin(), i.end(), [&](int x, int y) {
            if (unordered_items[x] != unordered_items[y])
                return unordered_items[x] > unordered_items[y];
            else
                return x < y;
            });
    }

    /* build FP tree */
    size_t index = 0;
    TreeNode* root = new TreeNode(-1);
    for (auto& i : data) {
        TreeNode* cur = root;
        TreeNode* child;
        for (auto item : i) {
            child = cur->findChild(item);
            if (!child) {
                child = cur->addChild(item);
                child->parent = cur;
                header[item].push_back(child); // link list for item     
            }

            child->count += pattBaseCount[index];
            cur = child;
        }
        index++;
    }
    return root;
}

void* findPatterns_thread(void* _arg) {

    vector<Arg>* arg = (vector<Arg> *)_arg;
    vector<map<vector<int>, int>> thread_patterns;
    for (size_t i = 0;i < arg->size();i++)
    {
        map<vector<int>, int> local_patterns;
        findPatterns_serial((*arg)[i].node, (*arg)[i].header, &(*arg)[i].prefix, &local_patterns, (*arg)[i].header_count);
        thread_patterns.push_back(local_patterns);
    }
    pthread_mutex_lock(&mutex_pat);
    global_patterns.resize(global_patterns.size() + thread_patterns.size());
    global_patterns.insert(global_patterns.end(), thread_patterns.begin(), thread_patterns.end());
    pthread_mutex_unlock(&mutex_pat);
    pthread_exit(NULL);
}

void findPatterns_main(TreeNode* node, unordered_map<int, vector<TreeNode*>>& header, pattern prefix, unordered_map<int, int>& header_count) {

    int header_size = header.size();
    pthread_t p_thread[THREAD_COUNT];
    vector<vector<Arg>> thread_struct(THREAD_COUNT);
    bool has_created[THREAD_COUNT] = { false };
    int level = 0;
    int thread_id = 0;
    for (auto& head : header) {
        int item = head.first;
        pattern local_prefix = prefix;
        local_prefix.first.push_back(item);
        local_prefix.second = header_count[item];
        sort(local_prefix.first.begin(), local_prefix.first.end()); //WHY SORT ITEM
        map<vector<int>, int> temp={{local_prefix.first,local_prefix.second}};
        global_patterns.push_back(temp);

        vector<vector<int>> condPattBases;
        vector<int> pattBaseCount;
        int count = 0;
        for (auto n : head.second) {
            vector<int> path;
            TreeNode* parent = n->parent;
            while (parent != node) {
                path.push_back(parent->item);
                parent = parent->parent;
            }
            if (path.size() > 0) {
                condPattBases.push_back(path);
                pattBaseCount.push_back(n->count);
            }
        }
        unordered_map<int, vector<TreeNode*>> *new_header = new unordered_map<int, vector<TreeNode*>>;
        unordered_map<int, int> *new_header_count = new unordered_map<int, int> ;
        TreeNode* new_tree = createFpTree(condPattBases, *new_header, *new_header_count, pattBaseCount);
        if (!new_tree) continue;
        Arg arg;
        arg.header = new_header;
        arg.header_count = new_header_count;
        arg.node = new_tree;
        arg.prefix = local_prefix;
        thread_struct[thread_id].push_back(arg);
        has_created[thread_id] = true;
        thread_id = (thread_id + 1) % THREAD_COUNT;
    }
    for (int i = 0;i < THREAD_COUNT;i++) {
        if (has_created[i])
            pthread_create(&p_thread[i], NULL, findPatterns_thread, (void*)&thread_struct[i]);
    }
    for (int i = 0;i < THREAD_COUNT;i++) {
        if (has_created[i]) {
            pthread_join(p_thread[i], NULL);
        }
    }
    return;
}



void findPatterns_serial(TreeNode* node, unordered_map<int, vector<TreeNode*>> *header, pattern* prefix, map<vector<int>, int>* patterns, unordered_map<int, int> *header_count) {
    // printf("In findPatterns_serial beging\n");
    for (auto& head : *header) {
        int item = head.first;
        pattern local_prefix = *prefix;
        local_prefix.first.push_back(item);
        local_prefix.second = (*header_count)[item];
        sort(local_prefix.first.begin(), local_prefix.first.end()); //WHY SORT ITEM
        patterns->emplace(local_prefix.first, local_prefix.second);

        vector<vector<int>> condPattBases;
        vector<int> pattBaseCount;
        for (auto n : head.second) {
            vector<int> path;
            TreeNode* parent = n->parent;
            while (parent != node) {
                path.push_back(parent->item);
                parent = parent->parent;
            }
            if (path.size() > 0) {
                condPattBases.push_back(path);
                pattBaseCount.push_back(n->count);
            }
        }

        unordered_map<int, vector<TreeNode*>> *new_header =new unordered_map<int, vector<TreeNode*>>;
        unordered_map<int, int> *new_header_count = new unordered_map<int, int> ;
        TreeNode* new_tree = createFpTree(condPattBases, *new_header, *new_header_count, pattBaseCount);

        if (new_tree)
            findPatterns_serial(new_tree, new_header, &local_prefix, patterns, new_header_count);
    }
}


void printFpTree(TreeNode* root) {
    if (!root)
        return;

    int item = -1;
    if (root->parent)
        item = root->parent->item;
    cout << "item: " << root->item << " p: " << item << " count: " << root->count << "\n";
    for (auto& i : root->children)
        printFpTree(i.second);
}
