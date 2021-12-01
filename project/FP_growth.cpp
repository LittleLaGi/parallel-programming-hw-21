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
//void cleanUp(TreeNode* t, int min_support);
void freeTreeNodes(TreeNode* root);
void printFpTree(TreeNode* root);
TreeNode* createFpTree(vector<vector<int>>& data, int min_support, unordered_map<int, vector<TreeNode*>>& header, unordered_map<int, int>& header_count, vector<int>& pattBaseCount);
TreeNode* createCondFpTree(TreeNode* root, vector<TreeNode*>& head, int min_support);
void findPatterns(TreeNode* node, unordered_map<int, vector<TreeNode*>> header, pattern prefix, map<vector<int>, int>& patterns, int min_support, unordered_map<int, int>& header_count);


unordered_map<int, int> unordered_items;  // key: item, value: amount of the item
int main(int argc, char *argv[]) {
    int min_support;

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
    TreeNode* root = createFpTree(data, min_support, header, header_count, dummy);
    printFpTree(root);
    
    cout << '\n';
    for (auto i : header) {
            cout << i.first << " " << i.second.size() << " ";
            cout << header_count.find(i.first)->second << '\n';
    }

    /* sort the header: larger count first */
    // cout << "---> sort header\n";
    // vector<pair<int, vector<TreeNode*>>> ordered_header(header.begin(), header.end());
    // sort(ordered_header.begin(), ordered_header.end(), [&](auto i, auto j){
    //     int x = i.first;
    //     int y = j.first;
    //     if (unordered_items[x] != unordered_items[y])
    //         return unordered_items[x] > unordered_items[y];
    //     else
    //         return x < y;
    // });

    auto end_FpTree = std::chrono::high_resolution_clock::now();

    /* find freq patterns */
    cout << "---> find freq patterns\n";
    map<vector<int>, int> patterns;
    pattern pat;
    findPatterns(root, header, pat, patterns, min_support, header_count);

    auto end_findPattern = std::chrono::high_resolution_clock::now();

    /* dump the result */
    cout << "---> dump the result\n";
    ofstream outputFile;
    outputFile.open ("result.txt");
    for (auto& p : patterns) {
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
        //delete t2->parent;

    while (t1->getChildrenCount() && t2->getChildrenCount()) {
        auto p = t1->findChild(t2->item);
        if (!p)
            break;

        p->count += t2->count;
        t1 = p;
        t2 = t2->children.begin()->second;
        //delete t2->parent;
    }
    t1->addChild(t2);
    t2->parent = t1;
}

TreeNode* createFpTree(vector<vector<int>>& data, int min_support, unordered_map<int, vector<TreeNode*>>& header,
                       unordered_map<int, int>& header_count, vector<int>& pattBaseCount) {
    cout << "build FP tree\n";
    /* remove items with amount smaller than min support */
    unordered_map<int, int> data_count;
    for (size_t i = 0; i < data.size(); ++i) {
        for (auto j : data[i]) {
            auto iter = data_count.find(j);
            if (iter == data_count.end())
                data_count[j] = pattBaseCount[i];
            else
                data_count[j] += pattBaseCount[i];
        }
    }
    for (auto& i : data) {
        for (auto iter = i.begin(); iter != i.end();) {
            if (data_count[*iter] < min_support)
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
    

    for (auto i : data) {
        for (auto item : i)
            cout << item <<" ";
        cout<<"\n";
    }


    /* build FP tree */
    size_t index = 0;
    TreeNode* root = new TreeNode(-1);   
    for (auto& i : data) {
        TreeNode* cur = root;
        TreeNode* child;
        for (auto item : i) {
            if (header_count.find(item) == header_count.end())
                header_count[item] = 0;
            if (!(child = cur->findChild(item))) {
                child = cur->addChild(item);
                child->parent = cur;
                header[item].push_back(child);      
            }

            child->count += pattBaseCount[index];
            header_count[item] += pattBaseCount[index];
            cur = child;
        }
        index++;
    }

    // auto iter = header_count.begin();
    // while (iter != header_count.end()) {
    //     if (iter->second < min_support) {
    //         header.erase(iter->first);
    //         iter = header_count.erase(iter);
    //     }
    //     else
    //         ++iter;
    // }

    return root;
}

// TreeNode* createCondFpTree(TreeNode* root, vector<TreeNode*>& head, int min_support) {
//     vector<TreeNode*> paths;
//     for (auto h : head) {
//         TreeNode* hold;
//         TreeNode* cur = h;
//         TreeNode* parent = cur->parent;
//         while (parent != root) { 
//             hold = new TreeNode(parent->item);
//             hold->count = h->count;
//             if (cur != h) {
//                 hold->addChild(cur);
//                 cur->parent = hold;
//             }
//             cur = hold;
//             parent = parent->parent;
//         }
//         if (cur != h) {
//             hold = new TreeNode(-1);
//             hold->addChild(cur);
//             cur->parent = hold;
//             paths.push_back(hold);
//         }
//     }

//     if (paths.size() == 0)
//         return nullptr;

//     for (int i = 1; i < paths.size(); ++i)
//         mergeTreeNodes(paths[0], paths[i]);

//     cleanUp(paths[0], min_support);

//     return paths[0];
// }

void findPatterns(TreeNode* node, unordered_map<int, vector<TreeNode*>> header, pattern prefix, map<vector<int>, int>& patterns, int min_support,
                  unordered_map<int, int>& header_count) {
                      
    for (auto& head : header) {
        int item = head.first;
        pattern local_prefix = prefix;
        local_prefix.first.push_back(item);
        local_prefix.second = header_count[item];
        auto iter = patterns.find(local_prefix.first);
        if (iter != patterns.end())
            iter->second += local_prefix.second;
        else
            patterns.emplace(local_prefix.first, local_prefix.second);

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



            if (path.size() > 0){
                cout<< "--- path ---\n";
                for (auto i : path)
                    cout<< i<<' ';
                cout <<" -> " << n->count;
                cout<< "\n------\n";
            }

        }


        cout <<"item: "<< item << '\n';
        unordered_map<int, vector<TreeNode*>> new_header;
        unordered_map<int, int> new_header_count;
        TreeNode* new_tree = createFpTree(condPattBases, min_support, new_header, new_header_count, pattBaseCount);

        
        printFpTree(new_tree);
        cout << '\n';
        for (auto i : new_header) {
            cout << i.first << " " << i.second.size() << " ";
            cout << new_header_count.find(i.first)->second << '\n';
        }

        if (new_tree)
            findPatterns(new_tree, new_header, local_prefix, patterns, min_support, new_header_count);
    }
}

// void cleanUp(TreeNode* t, int min_support) {
//     if (!t)
//         return;
//     if (t->item != -1 && t->count < min_support) {
//         t->parent->children.erase(t->item);
//         freeTreeNodes(t);
//         return;
//     }
    
//     if (!t->getChildrenCount())
//         return;

//     for (auto& i : t->children)
//         cleanUp(i.second, min_support);
// }

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
        //delete root;
    
    for (auto& i : root->children)
        freeTreeNodes(i.second);
}