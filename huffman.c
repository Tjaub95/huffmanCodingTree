#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define len(x) ((int)log10(x)+1)

const int ALPHABET_W_SPACE = 27;

/* Node of the huffman tree */
struct node
{
    int value;
    char letter;
    struct node *left,*right;
};

typedef struct node Node;

/* 81 = 8.1%, 128 = 12.8% and so on. The 27th frequency is the space. Source is Wikipedia */
int english_letter_frequencies [27] = {81, 15, 28, 43, 128, 23, 20, 61, 71, 2, 1, 40, 24, 69, 76, 20, 1, 61, 64, 91, 28, 10, 24, 1, 20, 1, 130};

/*finds and returns the small sub-tree in the forrest*/
int find_smaller (Node *array[], int different_from)
{
    int smaller;
    int i = 0;

    while (array[i]->value == -1)
        i++;
    smaller = i;
    if (i == different_from){
        i++;
        while (array[i]->value == -1)
            i++;
        smaller = i;
    }

    for (i = 1; i < ALPHABET_W_SPACE; i++){
        if (array[i]->value == -1)
            continue;
        if (i == different_from)
            continue;
        if (array[i]->value < array[smaller]->value)
            smaller = i;
    }

    return smaller;
}

/*builds the huffman tree and returns its address by reference*/
void build_huffman_tree(Node **tree)
{
    Node *temp;
    Node *array[ALPHABET_W_SPACE];
    int i, sub_trees = ALPHABET_W_SPACE;
    int small_one;
    int small_two;

    for (i = 0; i < ALPHABET_W_SPACE; i++)
    {
        array[i] = malloc(sizeof(Node));
        array[i]->value = english_letter_frequencies[i];
        array[i]->letter = i;
        array[i]->left = NULL;
        array[i]->right = NULL;
    }

    while (sub_trees > 1)
    {
        small_one=find_smaller(array,-1);
        small_two=find_smaller(array,small_one);
        temp = array[small_one];
        array[small_one] = malloc(sizeof(Node));
        array[small_one]->value = temp->value + array[small_two]->value;
        array[small_one]->letter = 127;
        array[small_one]->left = array[small_two];
        array[small_one]->right = temp;
        array[small_two]->value = -1;
        sub_trees--;
    }

    *tree = array[small_one];

    return;
}

/* builds the table with the bits for each letter. 1 stands for binary 0 and 2 for binary 1 (used to facilitate arithmetic)*/
void fill_table(int code_table[], Node *tree, int code)
{
    if (tree->letter < ALPHABET_W_SPACE)
        code_table[(int)tree->letter] = code;
    else
    {
        fill_table(code_table, tree->left, code*10+1);
        fill_table(code_table, tree->right, code*10+2);
    }

    return;
}

/*invert the codes in codeTable2 so they can be used with mod operator by compressFile function*/
void invert_codes(int code_table_one[], int code_table_two[])
{
    int i;
    int n;
    int copy;

    for (i = 0; i < ALPHABET_W_SPACE; i++)
    {
        n = code_table_one[i];
        copy = 0;
        while (n > 0)
        {
            copy = copy * 10 + n %10;
            n /= 10;
        }
        code_table_two[i] = copy;
    }

    return;
}

void decode(char compressed_text[], Node *tree)
{
    Node *current_tree = tree;
    char c;
    char bit;
    char mask = 1 << 7;
    int i;

    char decompressed_text[strlen(compressed_text)];
    int decompressed_next = 0;

    int index = 0;
    while (index < strlen(compressed_text))
    {
        for (i = 0; i < 8; i++)
        {
            bit = c & mask;
            c = c << 1;
            if (bit == 0)
            {
                current_tree = current_tree->left;
                if (current_tree->letter != 127)
                {
                    if (current_tree->letter == 26) {
                        decompressed_text[decompressed_next] = 32;
                        decompressed_next++;
                    }
                    else
                    {
                        decompressed_text[decompressed_next] = current_tree->letter + 97;
                        decompressed_next++;
                    }
                    current_tree = tree;
                }
            }
            else
            {
                current_tree = current_tree->right;
                if (current_tree->letter != 127)
                {
                    if (current_tree->letter == 27)
                    {
                        decompressed_text[decompressed_next] = 32;
                        decompressed_next++;
                    }
                    else
                    {
                        decompressed_text[decompressed_next] = current_tree->letter + 97;
                        decompressed_next++;
                    }
                    current_tree = tree;
                }
            }
        }
        index++;
    }

    fprintf(stderr, "\nDecompressed Text:\n%s\n", decompressed_text);

    return;
}

void encode(char buffer[], int code_table[], Node *tree) {
    char bit;
    char x = 0;
    int n;
    int length;
    int bits_left = 8;
    int original_bits = 0;
    int compressed_bits = 0;

    char compressed_text[strlen(buffer)];
    int compressed_next = 0;

    int i = 0;
    while (i < strlen(buffer))
    {
        original_bits++;
        if (buffer[i] == 32)
        {
            length = len(code_table[26]);
            n = code_table[26];
        }
        else
        {
            length = len(code_table[buffer[i] - 97]);
            n = code_table[buffer[i] - 97];
        }

        while (length > 0)
        {
            compressed_bits++;
            bit = n % 10 - 1;
            n /= 10;
            x = x | bit;
            bits_left--;
            length--;
            if (bits_left == 0)
            {
                compressed_text[compressed_next] = x;
                compressed_next++;
                x = 0;
                bits_left = 8;
            }

            x = x << 1;
        }
        i++;
    }

    if (bits_left != 8)
    {
        x = x << (bits_left-1);
        compressed_text[compressed_next] = x;
    }

    // Print details of compression to console.
    fprintf(stderr,"Original bits = %d\n",original_bits*8);
    fprintf(stderr,"Compressed bits = %d\n",compressed_bits);
    fprintf(stderr,"Saved %.2f%% of memory\n",((float)compressed_bits/(original_bits*8))*100);
    fprintf(stderr,"\nOriginal Text:\n%s\n", buffer);
    fprintf(stderr,"\nCompressed Text:\n%s\n", compressed_text);

    decode(compressed_text, tree);

    return;
}

int main()
{
    Node *huffman_tree;
    int code_table_one[ALPHABET_W_SPACE];
    int code_table_two[ALPHABET_W_SPACE];
    char buffer[] = {'e', 'n', 'q', 'u', 'e', 'u', 'e', '\0'};

    build_huffman_tree(&huffman_tree);

    fill_table(code_table_one, huffman_tree, 0);

    invert_codes(code_table_one, code_table_two);

    // Get user input compress it and output it, decompress the compression and output that.
    encode(buffer, code_table_two, huffman_tree);
}
