#include "FP_growth.hpp"

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

using namespace std;
using pattern = pair<vector<int>, int>;

void readData(vector<vector<int>>& data);
void mergeTreeNodes(TreeNode* t1, TreeNode* t2);
void cleanUp(TreeNode* t, int min_support);
void freeTreeNodes(TreeNode* root);
void printFpTree(TreeNode* root);
TreeNode* createFpTree(vector<vector<int>>& data, int min_support, unordered_map<int, vector<TreeNode*>>& header);
TreeNode* createCondFpTree(TreeNode* root, vector<TreeNode*>& head, int min_support);
void findPatterns(TreeNode* root, pattern prefix, map<vector<int>, int>& patterns, int item);

int main(int argc, char *argv[]) {
    int min_support;

    if (argc != 2) {
        cout << "[ERROR] must have two arguments !!\n";
        exit(1);
    }
    min_support = atoi(argv[1]);
    
    vector<vector<int>> data;
    unordered_map<int, int> unordered_items;  // key: item, value: amount of the item

    /* read data and count the amount of each item */
    readData(data);
    for (auto& i : data) {
        for (auto item : i) {
            ++unordered_items[item];
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    /* remove items with amount smaller than min support */
    unordered_set<int> to_delete;
    for (auto iter = unordered_items.begin(); iter != unordered_items.end(); ++iter) {
        if (iter->second < min_support) {
            to_delete.insert(iter->first);
            unordered_items.erase(iter);
        }
    }
    for (auto& i : data) {
        for (auto iter = i.begin(); iter != i.end();) {
            if (to_delete.find(*iter) != to_delete.end())
                iter = i.erase(iter);
            else
                ++iter;
        }
    }

    /* sort items in each data with their amount in descendent order */
    for (auto& i : data) {
        sort(i.begin(), i.end(), [&](int x, int y){
            if (unordered_items[x] != unordered_items[y])
                return unordered_items[x] > unordered_items[y];
            else
                return x < y;
        });
    }

    auto end_preprocess = std::chrono::high_resolution_clock::now();

    /* build FP tree */
    unordered_map<int, vector<TreeNode*>> header; // key: item, value: a list of TreeNodes
    TreeNode* root = createFpTree(data, min_support, header);

    /* sort the header: larger count first */
    vector<pair<int, vector<TreeNode*>>> ordered_header(header.begin(), header.end());
    sort(ordered_header.begin(), ordered_header.end(), [&](auto i, auto j){
        int x = i.first;
        int y = j.first;
        if (unordered_items[x] != unordered_items[y])
            return unordered_items[x] > unordered_items[y];
        else
            return x < y;
    });

    auto end_FpTree = std::chrono::high_resolution_clock::now();

   /* find freq patterns */
   map<vector<int>, int> patterns;
    for (auto& head : ordered_header) {
         /* build conditional FP tree */
        TreeNode* local_root = createCondFpTree(root, head.second, min_support);
        
        /* find patterns */
        findPatterns(local_root, {}, patterns, head.first);
    }

    for (auto& i : unordered_items)
        patterns[{i.first}] = i.second;

    auto end_findPattern = std::chrono::high_resolution_clock::now();

    /* dump the result */
    ofstream outputFile;
    outputFile.open ("result.txt");
    for (auto& p : patterns) {
        for (auto item : p.first)
            outputFile << item << " ";
        outputFile << ": " << p.second << "\n";
    }
    outputFile.close();

    /* print out timing info */
    std::chrono::duration<double> elapsed1 = end_preprocess - start;
    std::chrono::duration<double> elapsed2 = end_FpTree - end_preprocess;
    std::chrono::duration<double> elapsed3 = end_findPattern - end_FpTree;
    cout << '\n';
    cout << "----- [Time] -----\n";
    cout << "total: " << elapsed1.count() + elapsed2.count() + elapsed3.count() << '\n';
    cout << "preprocess: " << elapsed1.count() << '\n';
    cout << "FpTree: " << elapsed1.count() << '\n';
    cout << "freq pattern: " << elapsed1.count() << '\n';
    cout << "------------------\n\n";

    return 0;
}

void readData(vector<vector<int>>& data){
    int id = -1, item;
    data.push_back({});
    while(cin >> id >> id >> item) {
        if (id == data.size())
            data.push_back({});
        data[id].push_back(item);
    }
}

void mergeTreeNodes(TreeNode* t1, TreeNode* t2) {
    t2 = t2->children.begin()->second;
    if (t2->parent->item == -1)
        delete t2->parent;

    while (t1->getChildrenCount() && t2->getChildrenCount()) {
        auto p = t1->findChild(t2->item);
        if (!p)
            break;

        p->count += t2->count;
        t1 = p;
        t2 = t2->children.begin()->second;
        delete t2->parent;
    }
    t1->addChild(t2);
    t2->parent = t1;
}

TreeNode* createFpTree(vector<vector<int>>& data, int min_support, unordered_map<int, vector<TreeNode*>>& header) {
    TreeNode* root = new TreeNode(-1);   
    for (auto& i : data) {
        TreeNode* cur = root;
        TreeNode* child;
        for (auto item : i) {
            if (child = cur->findChild(item)) {
                child->count++;           
            }
            else {
                child = cur->addChild(item);
                child->count++;
                child->parent = cur;
                header[item].push_back(child);
            }
            cur = child;
        }
    }

    return root;
}

TreeNode* createCondFpTree(TreeNode* root, vector<TreeNode*>& head, int min_support) {
    vector<TreeNode*> paths;
    for (auto h : head) {
        TreeNode* hold;
        TreeNode* cur = h;
        TreeNode* parent = cur->parent;
        while (parent != root) { 
            hold = new TreeNode(parent->item);
            hold->count = h->count;
            if (cur != h) {
                hold->addChild(cur);
                cur->parent = hold;
            }
            cur = hold;
            parent = parent->parent;
        }
        if (cur != h) {
            hold = new TreeNode(-1);
            hold->addChild(cur);
            cur->parent = hold;
            paths.push_back(hold);
        }
    }

    if (paths.size() == 0)
        return nullptr;

    for (int i = 1; i < paths.size(); ++i)
        mergeTreeNodes(paths[0], paths[i]);

    cleanUp(paths[0], min_support);

    return paths[0];
}

void findPatterns(TreeNode* root, pattern prefix, map<vector<int>, int>& patterns, int item) {
    if (!root)
        return;
    
    if (root->getChildrenCount()) {
        // skip root->item
        for (auto& i : root->children)
            findPatterns(i.second, prefix, patterns, item);

        if (root->item == -1)
            return;

        // put root->item in prefix
        prefix.second = root->count;
        prefix.first.push_back(root->item);
        for (auto& i : root->children)
            findPatterns(i.second, prefix, patterns, item);
    }
    else {
        // skip root->item
        auto tmp = prefix;
        tmp.first.push_back(item);
        auto iter = patterns.find(tmp.first);
        if (iter != patterns.end())
            iter->second += tmp.second;
        else
            patterns[tmp.first] = tmp.second;

        // put root->item in prefix
        prefix.second = root->count;
        prefix.first.push_back(root->item);
        prefix.first.push_back(item);
        iter = patterns.find(prefix.first);
        if (iter != patterns.end())
            iter->second += prefix.second;
        else
            patterns[prefix.first] = prefix.second;
    }
}

void cleanUp(TreeNode* t, int min_support) {
    if (!t)
        return;
    
    if (t->item != -1 && t->count < min_support) {
        t->parent->children.erase(t->item);
        freeTreeNodes(t);
        return;
    }
    
    if (!t->getChildrenCount())
        return;

    for (auto& i : t->children)
        cleanUp(i.second, min_support);
}

void printFpTree(TreeNode* root) {
    if (!root)
        return;

    int item = -1;
    if (root->parent)
        item = root->parent->item;
    cout << "item: " << root->item << " p: " << item << " count: " <<root->count << "\n";
    for (auto& i : root->children)
        printFpTree(i.second);  
}

void freeTreeNodes(TreeNode* root) {
    if (!root->getChildrenCount())
        delete root;
    
    for (auto& i : root->children)
        freeTreeNodes(i.second);
}