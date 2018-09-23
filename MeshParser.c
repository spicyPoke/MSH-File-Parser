/* 
 * MSH File parser
 *
 * Copyright (C) 2018 Experian Elitiawan <experian.elitiawan@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
 * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */


/* Curently This Parser only support :
 * - up to 8 Nodes Hexahedron
 * - up to 32 letters name
 * - up to 10 tags per element 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum elementTypes
{
    _2_NODE_LINE = 1,
    _3_NODE_TRIANGLE,
    _4_NODE_QUADRANGLE,
    _4_NODE_TETRAHEDRON,
    _8_NODE_HEXAHEDRON,
    _6_NODE_PRISM,
    _5_NODE_PYRAMID,
    _1_NODE_POINT = 15
};

typedef struct 
{
    int dimension;
    int physicalNumber;
    char name[32];
}PhysicalNameInfo;
PhysicalNameInfo* physicalNamesInfo = NULL;

typedef struct
{
    int number;
    float x;
    float y;
    float z;
}NodeInfo;
NodeInfo* nodesInfo = NULL;

typedef struct
{
    int number;
    int type;
    int tagCount;
    int tags[10];
    int nodes[8];
}ElementInfo;
ElementInfo* elementsInfo = NULL;

//Indicator-Nodes-Element (INE) Info struct
typedef struct
{
    int indicator;
    int indicator2;
    int nodes[2];
    int element;
    int edgeNumber;
}IneInfo;
IneInfo* ineInfo = NULL;

//Element-to-Edge (El2Ed) Info struct
typedef struct
{
    int element;
    int edge1;
    int edge2;
    int edge3;
}El2EdInfo;
El2EdInfo* el2edInfo = NULL;
El2EdInfo* el2edInfo_output = NULL;

int versionCodeMacro = 0;
int versionCodeMicro = 0;
int fileType = 0;
int dataSize = 0;

int nodesCount = 0;
int elementsCount = 0;
int ineArrayLength = 0;
int physicalNamesCount = 0;


const char* physicalNameKeyword = "$PhysicalNames";
const char* physicalNameEndKeyword = "$EndPhysicalNames";

int compare_indicator(const void* a, const void* b)
{
    return ((IneInfo*)a)->indicator - ((IneInfo*)b)->indicator;
}

int compare_element(const void* a, const void* b)
{
    return ((IneInfo*)a)->element - ((IneInfo*)b)->element;
}

int count_string_length(const char* str)
{
    int i;
    int length = 0;
    for(i = 0; str[i] != '\0'; i++)
    {
        length++;
    }
    return length;
}

// !!IMPORTANT!!
// This function must be called before exiting the program or before starting new file reading
// Otherwise memory leak will occur
void free_memory_()
{
    if(nodesInfo)
    {
        printf("Freeing nodesInfo...\n");
        free(nodesInfo);
        nodesInfo = NULL;
    }

    if(physicalNamesInfo)
    {
        printf("Freeing physicalNamesInfo...\n");
        free(physicalNamesInfo);
        physicalNamesInfo = NULL;
    }

    if(elementsInfo)
    {
        printf("Freeing elementsInfo...\n");
        free(elementsInfo);
        elementsInfo = NULL;
    }

    if(ineInfo)
    {
        printf("Freeing ineInfo...\n");
        free(ineInfo);
        ineInfo = NULL;
    }

    if(el2edInfo)
    {
        printf("Freeing el2edInfo...\n");
        free(el2edInfo);
        el2edInfo = NULL;
    }

    if(el2edInfo_output)
    {
        printf("Freeing el2edInfo_output...\n");
        free(el2edInfo_output);
        el2edInfo_output = NULL;
    }
}

void parse_input_file_(int* out_physicalNamesCount, int* out_nodesCount, int* out_elementsCount)
{
    //Arbitrary variables (for for-loops and such)
    int i, j, k;
    //Temporary string to store region name 
    char currentRegionName[32];
    int seekPos = 0;

    //Make sure that there's no memory leak
    free_memory_();
    FILE* file = fopen("./../Input/rectangular baru.msh", "r");
    if(file)
    {
        printf("START READING FILE...\n");
        //File format
        fscanf(file, "$MeshFormat\n");
        fscanf(file, "%d.%d %d %d\n", &versionCodeMacro, &versionCodeMicro, &fileType, &dataSize);
        printf("==============================================\n");
        printf("File version            : %d.%d\n", versionCodeMacro, versionCodeMicro);
        printf("File Type               : %s\n", fileType == 0 ? "ASCII MSH File" : "Binary MSH File");
        printf("Atomic Data Size        : %d byte(s)\n", dataSize);
        printf("==============================================\n");
        fscanf(file, "$EndMeshFormat\n");
        
        //Physical Names (This region may not exist, thus a check is required)
        seekPos = ftell(file);
        fscanf(file, "%s\n", currentRegionName);
        if(strcmp(currentRegionName, physicalNameKeyword) == 0)
        {
            fscanf(file, "%d\n", &physicalNamesCount);
            *out_physicalNamesCount = physicalNamesCount;
            printf("Physical Names Count    : %d\n", physicalNamesCount);
            physicalNamesInfo = (PhysicalNameInfo*) calloc(physicalNamesCount, sizeof(PhysicalNameInfo));
            for(i = 0; i < physicalNamesCount; i++)
            {
                fscanf(file, "%d %d %s\n", &(physicalNamesInfo[i].dimension), &(physicalNamesInfo[i].physicalNumber), physicalNamesInfo[i].name);
            }
            fscanf(file, "$EndPhysicalNames\n");
        }
        else
        {
            printf("Physical Names Count    : %d\n", 0);
            fseek(file, seekPos, SEEK_SET);
        }

        //Nodes
        fscanf(file, "$Nodes\n");
        fscanf(file, "%d\n", &nodesCount);
        *out_nodesCount = nodesCount;
        printf("Nodes Count             : %d\n", nodesCount);
        nodesInfo = (NodeInfo*) calloc(nodesCount, sizeof(NodeInfo));
        for(i = 0; i < nodesCount; i++)
        {
            fscanf(file, "%d %f %f %f\n", &(nodesInfo[i].number), &(nodesInfo[i].x),&(nodesInfo[i].y), &(nodesInfo[i].z)); 
        }
        fscanf(file, "$EndNodes\n");
        
        //Elements
        fscanf(file, "$Elements\n");
        fscanf(file, "%d\n", &elementsCount);
        *out_elementsCount = elementsCount;
        printf("Elements Count          : %d\n", elementsCount);
        elementsInfo = (ElementInfo*) calloc(elementsCount, sizeof(ElementInfo));
        for(i = 0; i < elementsCount; i++)
        {
            fscanf(file, "%d", &(elementsInfo[i].number));
            fscanf(file, "%d", &(elementsInfo[i].type));
            fscanf(file, "%d", &(elementsInfo[i].tagCount));
            for(j = 0; j < elementsInfo[i].tagCount; j++)
            {
                fscanf(file, "%d", &(elementsInfo[i].tags[j]));
            }
            switch(elementsInfo[i].type)
            {
                case _1_NODE_POINT:
                {
                    for(k = 0; k < 1; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _2_NODE_LINE:
                {
                    for(k = 0; k < 2; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _3_NODE_TRIANGLE:
                {
                    for(k = 0; k < 3; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _4_NODE_QUADRANGLE:
                {
                    for(k = 0; k < 4; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _4_NODE_TETRAHEDRON:
                {
                    for(k = 0; k < 4; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _5_NODE_PYRAMID:
                {
                    for(k = 0; k < 5; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _6_NODE_PRISM:
                {
                    for(k = 0; k < 6; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
                case _8_NODE_HEXAHEDRON:
                {
                    for(k = 0; k < 8; k++)
                    {
                        fscanf(file, "%d", &(elementsInfo[i].nodes[k]));
                    }
                }break;
            }
        }
        fscanf(file, "$EndElements\n");
        
        //End file operation
        fclose(file);

        printf("==============================================\n");
        printf("READ SUCCESS...\n");
    }
    else
    {
        printf("!!!ERROR OPENING FILE!!!\n");
        printf("Please check that the directory is correct and the file does exist\n");
    }
}

void retrieve_node_info_(int* index, int* node_num, float* node_x, float* node_y, float* node_z)
{
    *node_num = nodesInfo[*index - 1].number;
    *node_x = nodesInfo[*index - 1].x;
    *node_y = nodesInfo[*index - 1].y;
    *node_z = nodesInfo[*index - 1].z;
}

void retrieve_physical_name_info_(int* index, int* physical_name_dimension, int* physical_name_number, char* name)
{
    int i;
    *physical_name_dimension = physicalNamesInfo[*index - 1].dimension;
    *physical_name_number = physicalNamesInfo[*index - 1].physicalNumber;
    for(i = 0; i < 32; i++)
    {
        name[i] = physicalNamesInfo[*index - 1].name[i];
    }
}

void retrieve_element_info_(int* index, int* element_number, int* element_type, int* element_tagCount, int* element_tags, int* element_nodes)
{
    int i;
    *element_number = elementsInfo[*index - 1].number;
    *element_type = elementsInfo[*index - 1].type;
    *element_tagCount = elementsInfo[*index - 1].tagCount;
    for(i = 0; i < 10; i++)
    {
        element_tags[i] = elementsInfo[*index - 1].tags[i];
    }
    
    for(i = 0; i < 8; i++)
    {
        element_nodes[i] = elementsInfo[*index - 1].nodes[i];
    }
}

void retrieve_ine_array_(int* index, int* ine_indicator, int* ine_nodes, int* ine_element, int* ine_edgeNumber)
{
    *ine_indicator = ineInfo[*index - 1].indicator;
    ine_nodes[0] = ineInfo[*index - 1].nodes[0];
    ine_nodes[1] = ineInfo[*index - 1].nodes[1];
    *ine_element = ineInfo[*index - 1].element;
    *ine_edgeNumber = ineInfo[*index - 1].edgeNumber;
}

//Currently only works with _3_NODE_TRIANGLE
void create_ine_array_()
{
    int i, j;
    int k = 0;
    ineArrayLength = elementsCount * 3;
    ineInfo = (IneInfo*) calloc(ineArrayLength, sizeof(IneInfo));
    for(i = 0; i < elementsCount; i++)
    {
        if(elementsInfo[i].type == _3_NODE_TRIANGLE)
        {
            for(j = 0; j < 3; j++)
            {
                ineInfo[k + j].element = elementsInfo[i].number;
                ineInfo[k + j].nodes[0] = elementsInfo[i].nodes[j];
                ineInfo[k + j].nodes[1] = j + 1 < 3 ? elementsInfo[i].nodes[j + 1] : elementsInfo[i].nodes[0];
                ineInfo[k + j].indicator = ineInfo[k + j].nodes[0] * ineInfo[k + j].nodes[1];
                ineInfo[k + j].indicator2 = ineInfo[k + j].nodes[0] + ineInfo[k + j].nodes[1];
            }
            k += 3;
        }
    }
}

void sort_ine_array_by_indicator_()
{
    qsort(ineInfo, ineArrayLength, sizeof(IneInfo), compare_indicator);
}

void sort_ine_array_by_element_()
{
    qsort(ineInfo, ineArrayLength, sizeof(IneInfo), compare_element);
}

void create_el2ed_array()
{
    int i = 0;
    el2edInfo = (El2EdInfo*) calloc(elementsCount, sizeof(El2EdInfo));
    for(i = 0; i < ineArrayLength; i += 3)
    {
        int element = ineInfo[i].element;
        if(element)
        {
            int edge1 = ineInfo[i].edgeNumber;
            int edge2 = 0;
            int edge3 = 0;
            if(ineInfo[i].nodes[1] == ineInfo[i + 1].nodes[0])
            {
                edge2 = ineInfo[i + 1].edgeNumber; 
                edge3 = ineInfo[i + 2].edgeNumber;
            }
            else
            {
                edge2 = ineInfo[i + 2].edgeNumber;
                edge3 = ineInfo[i + 1].edgeNumber;
            }
            el2edInfo[i / 3].element = element;
            el2edInfo[i / 3].edge1 = edge1;
            el2edInfo[i / 3].edge2 = edge2;
            el2edInfo[i / 3].edge3 = edge3;
        }
    }
}

void label_the_edges_()
{
    int i;
    int previousIndicator = 0;
    int previousIndicator2 = 0;
    int label = 0;
    create_ine_array_();
    sort_ine_array_by_indicator_();
    for(i = 0; i < ineArrayLength; i++)
    {
        if(ineInfo[i].indicator != previousIndicator || ineInfo[i].indicator2 != previousIndicator2)
        {
            previousIndicator = ineInfo[i].indicator;
            previousIndicator2 = ineInfo[i].indicator2;
            label++;
        }
        ineInfo[i].edgeNumber = label;
    }
    sort_ine_array_by_element_();
    create_el2ed_array();
}

// void retrieve_read_data_(PhysicalNameInfo* out_physicalNamesInfo, NodeInfo* out_nodesInfo, ElementInfo* out_elementsInfo)
// {
//     int i;
//     printf("Assigning nodes data to fortran...\n");
//     for(i = 0; i < nodesCount; i++)
//     {
//         printf("node %d: [%f, %f, %f]\n", nodesInfo[i].number, nodesInfo[i].x, nodesInfo[i].y, nodesInfo[i].z);
//     }
//     out_physicalNamesInfo = physicalNamesInfo;
//     out_nodesInfo = nodesInfo;
//     out_elementsInfo = elementsInfo;
// }

//This section is not required unless you want to debug the code
int main (int argc, char** argv)
{
    int i;
    int dummy;
    parse_input_file_(&dummy, &dummy, &dummy);
    label_the_edges_();
    for(i = 0; i < ineArrayLength; i++)
    {
        printf("%d\t%d\t%d\t%d\n", ineInfo[i].element, ineInfo[i].nodes[0], ineInfo[i].nodes[1], ineInfo[i].edgeNumber);
    }
    printf("element\t\tedge number (1) (2) (3)\n");
    for(i = 0; i < elementsCount; i++)
    {
        printf("\t%d", el2edInfo[i].element);
        printf("\t\t%d %d %d\n", el2edInfo[i].edge1, el2edInfo[i].edge2, el2edInfo[i].edge3);
    }
}
