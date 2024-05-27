

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <list>
using namespace std;
using namespace std::chrono;

struct DocumentItem {
    string documentName;
    int count;
};

struct WordItem {
    string word;
    vector<DocumentItem> documents;

    // Add a new document or update the count if the document already exists
    void addDocument(const string& docName, int count) {
        auto it = find_if(documents.begin(), documents.end(), [&](const DocumentItem& item){
            return item.documentName == docName;
        });
        if (it != documents.end()) {
            it->count += count;
        } else {
            documents.push_back({docName, count});
        }
    }
};
class HashTable {
private:
    vector<list<WordItem>> table;
    int numElements;
    int capacity;

    // Prime number just less than the maximum possible size of the table
    static const int primeBase = 31;
    static const int primeMod = 1e9 + 9;

    int hashFunction(const string& key) {
        long long hashValue = 0;
        long long p = 1;
        for (char ch : key) {
            hashValue = (hashValue + (ch - 'a' + 1) * p) % primeMod;
            p = (p * primeBase) % primeMod;
        }
        return hashValue % capacity;
    }

    void rehash() {
        int oldCapacity = capacity;
        vector<list<WordItem>> oldTable = std::move(table);

        capacity *= 2;
        table.clear();
        table.resize(capacity);
        numElements = 0;

        for (auto& list : oldTable) {
            for (auto& item : list) {
                insert(item);
            }
        }

        cout << "Rehashed..." << endl;
        cout << "Previous table size: " << oldCapacity << ", new table size: " << capacity << endl;
    }

public:
    HashTable(int size = 101) : capacity(size), numElements(0) {
        table.resize(capacity);
    }

    void insert(const WordItem& item) {
        int index = hashFunction(item.word);
        bool found = false;

        for (auto& it : table[index]) {
            if (it.word == item.word) {
                found = true;
                for (auto& doc : item.documents) {
                    it.addDocument(doc.documentName, doc.count);
                }
                break;
            }
        }

        if (!found) {
            table[index].push_back(item);
            numElements++;
        }

        if (static_cast<double>(numElements) / capacity > 0.75) {
            rehash();
        }
    }

    list<WordItem>* find(const string& word) {
        int index = hashFunction(word);
        for (auto& item : table[index]) {
            if (item.word == word) {
                return &table[index];
            }
        }
        return nullptr;
    }

    int getNumElements() const {
        return numElements;
    }

    int getCurrentCapacity() const {
        return capacity;
    }
};


struct Node {
    WordItem data;
    int height; // Height of the node
    Node *lchild, *rchild;
}*root = nullptr;

// Function prototypes
int height(Node *p);
Node *LLRotation(Node *p);
Node *LRRotation(Node *p);
Node *RRRotation(Node *p);
Node *RLRotation(Node *p);
Node *RInsert(Node *p, const WordItem& key);
void updateNodeData(Node* node, const WordItem& key);
void Inorder(Node *p);
Node *Search(const string& key);
Node *inPre(Node *p);
Node *inSucc(Node *p);
int BF(Node *p); // Balance factor
Node *deleteNode(Node *p, const string& key);
void processFile(const string& fileName, Node*& root);

void QueryDocumentsBST(Node* node, const string& query) {
    stringstream ss(query);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        Node* result = Search(word);
        if (result) {
            cout << "in Document ";
            for (const auto& doc : result->data.documents) {
                cout << doc.documentName << ", " << word << " found " << doc.count << " times." << endl;
            }
        } else {
            cout << word << " not found." << endl;
        }
    }
}
void QueryDocumentsHT(HashTable& hashTable, const string& query) {
    unordered_map<string, unordered_map<string, int>> counts;  // Map to hold counts per word per document
    stringstream ss(query);
    string word;

    while (ss >> word) {
        list<WordItem>* items = hashTable.find(word);
        if (items) {
            for (auto& item : *items) {
                for (auto& doc : item.documents) {
                    counts[word][doc.documentName] += doc.count;
                }
            }
        }
    }

    for (auto& wordPair : counts) {
        for (auto& docPair : wordPair.second) {
            cout << "in Document " << docPair.first << ", " << wordPair.first << " found " << docPair.second << " times." << endl;
        }
    }
}


int main() {
    int numFiles;
    cout << "Enter number of input files: ";
    cin >> numFiles;

    vector<string> fileNames(numFiles);
    for (int i = 0; i < numFiles; i++) {
        cout << "Enter " << i + 1 << ". file name: ";
        cin >> fileNames[i];
    }

    HashTable hashTable(53);  // Initialize the hash table with the initial capacity

    // Read each file and process words
    for (const string& fileName : fileNames) {
        ifstream file(fileName);
        string word;

        while (file >> word) {
            // Only consider alphabetical characters; others are treated as separators
            string filteredWord;
            for (char c : word) {
                if (isalpha(c)) filteredWord.push_back(tolower(c));
            }

            if (!filteredWord.empty()) {
                WordItem newItem;
                newItem.word = filteredWord;
                newItem.addDocument(fileName, 1);

                // Insert into BST and Hash Table
                root = RInsert(root, newItem);
                hashTable.insert(newItem);
            }
        }
        file.close();
    }

    // Output the unique word count and current load ratio after preprocessing
    cout << "After preprocessing, the unique word count is " << hashTable.getNumElements()
         << ". Current load ratio is " << static_cast<double>(hashTable.getNumElements()) / hashTable.getCurrentCapacity() << endl;

    // Now query the structures
    cout << "Enter queried words in one line: ";
    cin.ignore();  // Clear the newline character left in the input buffer
    string queryLine;
    getline(cin, queryLine);

    // Processing and timing queries
    int k = 20;  // Number of iterations for timing

    // Timing for BST
    auto start = high_resolution_clock::now();
    for (int i = 0; i < k; i++) {
        QueryDocumentsBST(root, queryLine);
    }
    auto BSTTime = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count() / k;

    // Timing for HashTable
    start = high_resolution_clock::now();
    for (int i = 0; i < k; i++) {
        QueryDocumentsHT(hashTable, queryLine);
    }
    auto HTTime = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count() / k;

    // Output timing results
    cout << "Time (BST): " << BSTTime << " nanoseconds" << endl;
    cout << "Time (HashTable): " << HTTime << " nanoseconds" << endl;

    double speedUp = static_cast<double>(BSTTime) / HTTime;
    cout << "Speed Up (HashTable over BST): " << speedUp << "x" << endl;

    return 0;
}








int height(Node *p) {
    if (!p) return 0;
    return p->height;
}

Node* LLRotation(Node* p) {
    Node* pl = p->lchild;
    Node* plr = pl->rchild;

    // Perform rotation
    pl->rchild = p;
    p->lchild = plr;

    // Update heights
    p->height = max(height(p->lchild), height(p->rchild)) + 1;
    pl->height = max(height(pl->lchild), height(pl->rchild)) + 1;

    // Return new root
    return pl;
}

Node* RRRotation(Node* p) {
    Node* pr = p->rchild;
    Node* prl = pr->lchild;

    // Perform rotation
    pr->lchild = p;
    p->rchild = prl;

    // Update heights
    p->height = max(height(p->lchild), height(p->rchild)) + 1;
    pr->height = max(height(pr->lchild), height(pr->rchild)) + 1;

    // Return new root
    return pr;
}

Node* LRRotation(Node* p) {
    p->lchild = RRRotation(p->lchild);
    return LLRotation(p);
}

Node* RLRotation(Node* p) {
    p->rchild = LLRotation(p->rchild);
    return RRRotation(p);
}

Node* RInsert(Node* node, const WordItem& key) {
    if (node == nullptr) {
        Node* newNode = new Node();
        newNode->data = key;
        newNode->height = 1;  // New node is initially added at leaf
        newNode->lchild = newNode->rchild = nullptr;
        return newNode;
    }

    if (key.word < node->data.word) {
        node->lchild = RInsert(node->lchild, key);
    } else if (key.word > node->data.word) {
        node->rchild = RInsert(node->rchild, key);
    } else {
        // Update the existing node with the new data
        updateNodeData(node, key);
        return node;
    }

    // Update height of this ancestor node
    node->height = max(height(node->lchild), height(node->rchild)) + 1;

    // Get the balance factor of this ancestor node to check whether this node became unbalanced
    int balance = BF(node);

    // If this node becomes unbalanced, then there are 4 cases

    // Left Left Case
    if (balance > 1 && key.word < node->lchild->data.word) {
        return LLRotation(node);
    }

    // Right Right Case
    if (balance < -1 && key.word > node->rchild->data.word) {
        return RRRotation(node);
    }

    // Left Right Case
    if (balance > 1 && key.word > node->lchild->data.word) {
        node->lchild = RRRotation(node->lchild);
        return LLRotation(node);
    }

    // Right Left Case
    if (balance < -1 && key.word < node->rchild->data.word) {
        node->rchild = LLRotation(node->rchild);
        return RRRotation(node);
    }

    // Return the (unchanged) node pointer
    return node;
}

void updateNodeData(Node* node, const WordItem& key) {
    // Here you need to define how to update the node's data when the word already exists
    // For example, you might want to merge the document lists or update counts
    // This is just a placeholder implementation
    for (const auto& newDoc : key.documents) {
        bool found = false;
        for (auto& existingDoc : node->data.documents) {
            if (existingDoc.documentName == newDoc.documentName) {
                existingDoc.count += newDoc.count;
                found = true;
                break;
            }
        }
        if (!found) {
            node->data.documents.push_back(newDoc);
        }
    }
}

void Inorder(Node *p) {
    if (p) {
        Inorder(p->lchild);
        cout << "Word: " << p->data.word << " | Height: " << p->height << " | Balance Factor: " << BF(p) << " | Documents: ";
        for (const auto& doc : p->data.documents) {
            cout << "{" << doc.documentName << ", " << doc.count << "} ";
        }
        cout << endl;
        Inorder(p->rchild);
    }
}

Node *Search(const string& key) {
    Node *t = root;
    while (t != nullptr) {
        // Compare words instead of integers
        if (key == t->data.word)
            return t;
        else if (key < t->data.word)
            t = t->lchild;
        else
            t = t->rchild;
    }
    return nullptr;
}

int BF(Node *p) {
    if (!p) return 0;
    int leftHeight = p->lchild ? p->lchild->height : 0;
    int rightHeight = p->rchild ? p->rchild->height : 0;
    return leftHeight - rightHeight;
}

// Function to find the inorder predecessor of a node
Node *inPre(Node *p) {
    while (p && p->rchild != nullptr) {
        p = p->rchild;
    }
    return p;
}

// Function to find the inorder successor of a node
Node *inSucc(Node *p) {
    while (p && p->lchild != nullptr) {
        p = p->lchild;
    }
    return p;
}

Node *deleteNode(Node *p, const string& key) {
    if (p == nullptr) {
        return nullptr;
    }

    if (p->lchild == nullptr && p->rchild == nullptr) {
        if (p == root) {
            root = nullptr;
        }
        delete p;
        return nullptr;
    }

    if (key < p->data.word) {
        p->lchild = deleteNode(p->lchild, key);
    } else if (key > p->data.word) {
        p->rchild = deleteNode(p->rchild, key);
    } else {
        if (height(p->lchild) > height(p->rchild)) {
            Node *q = inPre(p->lchild);
            p->data = q->data;
            p->lchild = deleteNode(p->lchild, q->data.word);
        } else {
            Node *q = inSucc(p->rchild);
            p->data = q->data;
            p->rchild = deleteNode(p->rchild, q->data.word);
        }
    }

    p->height = max(height(p->lchild), height(p->rchild)) + 1;

    int balance = BF(p);

    if (balance > 1 && BF(p->lchild) >= 0) {
        return LLRotation(p);
    } else if (balance > 1 && BF(p->lchild) < 0) {
        return LRRotation(p);
    } else if (balance < -1 && BF(p->rchild) <= 0) {
        return RRRotation(p);
    } else if (balance < -1 && BF(p->rchild) > 0) {
        return RLRotation(p);
    }

    return p;
}
void processFile(const string& fileName, Node*& root) {
    ifstream file(fileName);
    string word;

    while (file >> word) {
        // Convert to lowercase
        transform(word.begin(), word.end(), word.begin(), ::tolower);

        // Create or update the WordItem
        WordItem item;
        item.word = word;
        item.addDocument(fileName, 1);

        // Insert or update the AVL tree
        root = RInsert(root, item);
    }

    file.close();
}

