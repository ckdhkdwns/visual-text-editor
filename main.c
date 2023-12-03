#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#ifdef LINUX

#endif

#ifdef MACOS
#define BACKSPACE 127
#endif

#ifdef WINDOWS
#define BACKSPACE 8
#endif

#define BACKSPACE 127
#define ENTER 10
#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

#ifndef FN
#define FN(c) ((c) & 0x1B)
#endif

#define ESC 27

typedef struct Node
{
    struct Node *prev;
    struct Node *next;
    int data;
} Node;

typedef struct Position
{
    int x;
    int y;
    Node *current;
} Position;

typedef struct PNode
{ // find시에 탐색된 노드의 위치를 알기 위한 노드
    struct PNode *prev;
    struct PNode *next;
    Position *position;
} PNode;

typedef struct WindowSize
{
    int x;
    int y;
} WindowSize;

typedef struct DocumentInfo
{
    int lineCount;
    Node *frameFirstNode;
    Node *frameLastNode;
    int frameX;
    int frameY;
} DocumentInfo;

typedef struct fileInfo
{
    char *filename;
    char *filetype;
    bool isFileReading;
    bool isFileSaving;
    bool isUpdated;
    bool isNewFile;
} FileInfo;

Node *head;
Node *tail;
Position *position;
WindowSize *windowSize;
DocumentInfo *documentInfo;
FileInfo *fileInfo;

// just print;
void print(void);

// initalize
void initLinkedList(void);
void initCurses(void);
void initWindowSize(void);
void initDocumentInfo(void);
void initFileInfo(void);

// linked list
void insert(int data);
void delete(void);

// row length
int current_row_length(void);
int prev_row_length(void);
int next_row_length(void);

// key
void backspace(void);
void enter(void);
void arrowUp(void);
void arrowDown(void);
void arrowRight(void);
void arrowLeft(void);
void home(void);
void end(void);
void pageUp(void);
void pageDown(void);
void commonKey(int key);

// ctrl Functions
void save(void);
void quit(void);
void find(void);

// in order to save
void saveFileAsFilename(char *filename);

// in order to find
void highlight(PNode *p, char *word, int wordLength);
void printFindMessageBar(char *word, int currentResultIndex, int resultCount);
int countFindResult(PNode *wordListHead);
PNode *findWordsInDocument(char *word);

// move page frame
void moveFirstFrameRight(void);
void moveLastFrameLeft(void);
void moveFirstFrameLeft(void);
void moveLastFrameRight(void);

void initLinkedList(void)
{
    head = (Node *)malloc(sizeof(Node));
    tail = (Node *)malloc(sizeof(Node));
    position = (Position *)malloc(sizeof(Position));
    position->current = head;
    position->x = 0;
    position->y = 0;

    head->data = 0;
    tail->data = 0;

    head->next = tail;
    head->prev = head;
    tail->next = tail;
    tail->prev = head;
}

void initCurses(void)
{
    initscr();
    keypad(stdscr, TRUE);
    start_color();
    init_color(1, 0, 0, 0);
    init_color(2, 1000, 1000, 1000);
    init_pair(1, 1, 2);
    noecho();
    move(0, 0);
}

void initWindowSize(void)
{
    windowSize = (WindowSize *)malloc(sizeof(WindowSize));
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    windowSize->x = (int)w.ws_col;
    windowSize->y = (int)w.ws_row;
}

void initDocumentInfo(void)
{
    documentInfo = (DocumentInfo *)malloc(sizeof(DocumentInfo));
    documentInfo->lineCount = 1;

    documentInfo->frameFirstNode = (Node *)malloc(sizeof(Node));
    documentInfo->frameLastNode = (Node *)malloc(sizeof(Node));

    documentInfo->frameFirstNode = head;
    documentInfo->frameLastNode = tail;

    documentInfo->frameX = 0;
    documentInfo->frameY = 0;
}

void initFileInfo(void)
{
    fileInfo = (FileInfo *)malloc(sizeof(FileInfo));
    fileInfo->filename = "No name";
    fileInfo->filetype = "no ft";
    fileInfo->isFileReading = false;
    fileInfo->isFileSaving = false;
    fileInfo->isUpdated = false;
    fileInfo->isNewFile = true;
}

void insert(int data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    Node *p = position->current;

    new_node->data = data;
    new_node->next = p->next;
    new_node->prev = p;

    p->next->prev = new_node;
    p->next = new_node;

    position->current = new_node;
}

void delete()
{
    Node *prev = position->current->prev;
    position->current->prev->next = position->current->next;
    position->current->next->prev = prev;

    free(position->current);
    position->current = prev;
}

int current_row_length(void)
{
    Node *p1 = position->current;
    Node *p2 = position->current->next;
    // if (p1->data == ENTER && p1->next->data == ENTER)
    //     return 1;
    int count = 0;
    while ((p1->data != ENTER) && (p1 != head))
    {
        p1 = p1->prev;
        count++;
    }
    while ((p2->data != ENTER) && (p2 != tail))
    {
        p2 = p2->next;
        count++;
    }
    return count;
}

int prev_row_length(void)
{
    Node *p = position->current;
    int count = 0;
    while ((p->data != ENTER) && (p != head))
        p = p->prev;

    if (p == head)
        return -1;

    p = p->prev;
    while ((p->data != ENTER) && (p != head))
    {
        p = p->prev;
        count++;
    }
    return count;
}

int next_row_length(void)
{
    Node *p = position->current;

    int count = 0;
    if (p->data == ENTER)
        p = p->next;
    while ((p->data != ENTER) && (p != tail))
        p = p->next;

    if (p == tail)
        return -1;

    p = p->next;
    while ((p->data != ENTER) && (p != tail))
    {
        p = p->next;
        count++;
    }
    return count;
}

void print(void)
{
    if (fileInfo->isFileReading)
        return;
    clear();
    Node *p = documentInfo->frameFirstNode;

    int colCount = 0;
    int rowCount = 0;
    while (p->next != documentInfo->frameLastNode && p->next != tail)
    {
        if (p->next->data == ENTER)
        {
            colCount = 0;
            rowCount++;
        }
        else
        {
            if (colCount >= documentInfo->frameX && colCount < documentInfo->frameX + windowSize->x - 1)
            { // 커서가 frame안에 있어야지만 출력함
                mvaddch(rowCount, colCount - documentInfo->frameX, p->next->data);
            }
            colCount += 1;
        }
        p = p->next;
    }

    attron(COLOR_PAIR(1));

    char leftMessage[100];
    char rightMessage[100];

    if (fileInfo->isUpdated)
    {
        sprintf(leftMessage, "[%c%s] - %d lines",
                '*', fileInfo->filename, documentInfo->lineCount);
    }
    else
    {
        sprintf(leftMessage, "[%s] - %d lines",
                fileInfo->filename, documentInfo->lineCount);
    }

    sprintf(rightMessage, "%s | %d/%d",
            fileInfo->filetype,
            position->y + 1 + documentInfo->frameY,
            position->x + documentInfo->frameX);

    mvprintw(windowSize->y - 2, 0, "%s", leftMessage);
    mvprintw(windowSize->y - 2, windowSize->x - strlen(rightMessage), "%s", rightMessage);
    for (int i = strlen(leftMessage); i < windowSize->x - strlen(rightMessage); ++i)
    {
        mvaddch(windowSize->y - 2, i, ' ');
    }
    attroff(COLOR_PAIR(1));

    mvprintw(windowSize->y - 1, 0, "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    // 글이 없는 경우에는 ~표시를 하기
    for (int i = 0; i < windowSize->y - documentInfo->lineCount - 2; i++)
    {
        mvprintw(windowSize->y - 3 - i, 0, "~");
    }
    move(position->y, position->x);
}

void moveFirstFrameRight(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameFirstNode = documentInfo->frameFirstNode->next;
    while (documentInfo->frameFirstNode->data != ENTER)
    {
        documentInfo->frameFirstNode = documentInfo->frameFirstNode->next;
    }
}

void moveFirstFrameLeft(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameFirstNode = documentInfo->frameFirstNode->prev;
    while (documentInfo->frameFirstNode->data != ENTER && documentInfo->frameFirstNode != head)
    {
        documentInfo->frameFirstNode = documentInfo->frameFirstNode->prev;
    }
}

void moveLastFrameRight(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameLastNode = documentInfo->frameLastNode->next;
    while (documentInfo->frameLastNode->data != ENTER && documentInfo->frameLastNode != tail)
    {
        documentInfo->frameLastNode = documentInfo->frameLastNode->next;
    }
}

void moveLastFrameLeft(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameLastNode = documentInfo->frameLastNode->prev;
    while (documentInfo->frameLastNode->data != ENTER)
    {
        documentInfo->frameLastNode = documentInfo->frameLastNode->prev;
    }
}

void backspace(void)
{
    if (position->current == head)
        return;
    int prl = prev_row_length();
    int crl = current_row_length();
    if (position->x == 0 && position->y == 0)
    { // 페이지의 제일 처음인 경우 (문서 처음 x)
        if (crl > 0 && documentInfo->frameX > 0)
        { // 커서가 frame의 제일 처음인 경우
            documentInfo->frameX--;
        }
        else
        {
            position->y = 0;
            position->x = prl;
            moveFirstFrameLeft();
            moveLastFrameLeft();
            documentInfo->frameY--;
            documentInfo->lineCount--;
        }
    }
    else if (position->x == 0)
    { // 위 경우를 제외하고 커서가 제일 처음인 경우

        if (crl > 0 && documentInfo->frameX > 0)
        { // 커서가 frame의 제일 처음인 경우
            documentInfo->frameX--;
        }
        else
        { // 커서가 진짜 라인의 제일 처음인 경우
            if (documentInfo->frameLastNode == tail && documentInfo->frameY != 0)
            { // 페이지가 제일 아래로 내려가 있는 경우
                moveFirstFrameLeft();
                documentInfo->frameY--;
                position->x = prl;
            }
            else
            { // 페이지가 제일 아래가 아닌 경우
                position->y--;
                position->x = prl;
                moveLastFrameRight();
            }
            if (prl + 1 > windowSize->x)
            { // 이전 줄이 윈도우 x크기보다 긴 경우
                documentInfo->frameX = prl - windowSize->x + 1;
                position->x = windowSize->x - 1;
            }
            documentInfo->lineCount--;
        }
    }
    else
    {
        position->x--;
    }
    move(position->y, position->x);
    delete ();
    print();
}

void enter(void)
{
    insert(ENTER);
    documentInfo->lineCount++;
    documentInfo->frameX = 0;
    position->x = 0;
    if (documentInfo->lineCount >= windowSize->y - 1) // 문서의 라인 수가 화면 크기보다 클 경우
    {
        if (position->y == windowSize->y - 3)
        { // 커서가 화면의 제일 밑인 경우
            move(position->y, 0);
            moveFirstFrameRight();
            moveLastFrameRight();
            documentInfo->frameY++;
        }
        else
        {
            move(++position->y, 0);
            moveLastFrameLeft();
        }
    }
    else
    { // 이외의 경우
        move(++position->y, 0);
    }

    print();
}

void arrowUp(void)
{
    if (position->y == 0 && documentInfo->frameY == 0)
        return;

    int prl = prev_row_length();
    if (prl > position->x + documentInfo->frameX)
    { // 윗줄이 커서의 x값보다 긴 경우
        for (int i = 0; i < prl + 1; i++)
        {
            position->current = position->current->prev;
        }
        position->y--;
    }
    else
    { // 윗줄이 커서의 x값보다 짧은 경우

        for (int i = 0; i < position->x + documentInfo->frameX + 1; i++)
        {
            position->current = position->current->prev;
        }

        if (prl < documentInfo->frameX)
        {
            documentInfo->frameX = prl;
            position->x = 0;
        }
        else
        {
            position->x = prl - documentInfo->frameX;
        }
        position->y--;
    }
    if (position->y < 0)
    {
        position->y = 0;
        moveFirstFrameLeft();
        moveLastFrameLeft();
        documentInfo->frameY--;
    }
    move(position->y, position->x);
    print();
}

void arrowDown(void)
{

    if (position->y + documentInfo->frameY == documentInfo->lineCount - 1)
        return;
    int crl = current_row_length();
    int nrl = next_row_length();
    if (nrl > position->x + documentInfo->frameX)
    { // 아래 줄이 커서의 x값보다 긴 경우
        for (int i = 0; i < crl + 1; i++)
        {
            position->current = position->current->next;
        }
        position->y++;
    }
    else
    { // 아래 줄이 커서의 x값보다 짧은 경우
        for (int i = 0; i < crl - (position->x + documentInfo->frameX) + nrl + 1; i++)
        {
            position->current = position->current->next;
        }

        if (nrl < documentInfo->frameX)
        {
            documentInfo->frameX = nrl;
            position->x = 0;
        }
        else
        {
            position->y++;
            position->x = nrl - documentInfo->frameX;
        }
    }
    if (position->y > windowSize->y - 3)
    {
        position->y--;
        moveFirstFrameRight();
        moveLastFrameRight();
        documentInfo->frameY++;
    }
    move(position->y, position->x);
    print();
}

void arrowRight(void)
{
    if (position->current->next == tail)
        return;
    position->current = position->current->next;

    if (position->current->data == ENTER)
    {
        documentInfo->frameX = 0;
        position->y++;
        position->x = 0;
    }
    else if (position->x == windowSize->x - 2)
    {
        documentInfo->frameX++;
    }
    else
    {
        position->x++;
    }
    if (position->y == windowSize->y - 2)
    {
        position->y--;
        move(position->y, position->x);
        moveFirstFrameRight();
        moveLastFrameRight();
    }
    move(position->y, position->x);
    print();
}

void arrowLeft(void)
{
    if (position->current == head)
        return;
    if (position->current->data == ENTER)
    { // 라인의 제일 첫번째에서 왼쪽 화살표를 누른 경우
        int prl = prev_row_length();

        if (prl > windowSize->x - 1)
        {
            documentInfo->frameX = prl - windowSize->x - 1;
            position->x = windowSize->x - 1;
        }
        else
        {
            position->x = prl;
        }

        if (position->current == documentInfo->frameFirstNode)
        { // 페이지의 제일 첫 부분인 경우
            moveFirstFrameLeft();
            moveLastFrameLeft();
        }
        else
        {
            position->y--;
        }
    }
    else
    {
        position->x--;
    }
    move(position->y, position->x);
    position->current = position->current->prev;
    print();
}

void home(void)
{
    documentInfo->frameX = 0;

    while (position->current->data != ENTER && position->current != head)
    {
        position->current = position->current->prev;
    }
    position->x = 0;
    move(position->y, position->x);
    print();
}

void end(void)
{
    int crl = current_row_length();
    if (crl > windowSize->x)
    {
        documentInfo->frameX = crl - windowSize->x;
        position->x = windowSize->x - 1;
    }
    else
    {
        position->x = crl;
    }
    while (position->current->next->data != ENTER && position->current->next != tail)
    {
        position->current = position->current->next;
    }
    move(position->y, position->x);
    print();
}

void pageUp(void)
{

    if (documentInfo->frameFirstNode == head)
        return;
    if (documentInfo->frameY < windowSize->y - 2)
    {
        int lineCount = documentInfo->frameY;
        for (int i = 0; i < lineCount; i++)
        {
            moveFirstFrameLeft();
            moveLastFrameLeft();
        }
        documentInfo->frameY = 0;
    }
    else
    {
        documentInfo->frameLastNode = documentInfo->frameFirstNode;
        moveLastFrameRight();
        for (int i = 0; i < windowSize->y - 3; i++)
        {
            moveFirstFrameLeft();
        }
        documentInfo->frameY -= windowSize->y - 3;
    }
    position->current = documentInfo->frameFirstNode;
    position->x = 0;
    position->y = 0;

    move(position->y, position->x);
    print();
}

void pageDown(void)
{
    if (documentInfo->frameLastNode == tail)
        return;

    if (documentInfo->frameY + windowSize->y - 2 + windowSize->y - 2 > documentInfo->lineCount)
    { // 한 페이지 전체를 넘길 수 없는 경우
        int moveLineCount = documentInfo->lineCount - documentInfo->frameY - (windowSize->y - 2);
        for (int i = 0; i < moveLineCount; i++)
        {
            moveFirstFrameRight();
            moveLastFrameRight();
        }
        documentInfo->frameY += moveLineCount;
    }
    else
    { // 한 페이지 전체를 넘길 수 있음
        documentInfo->frameFirstNode = documentInfo->frameLastNode;
        moveFirstFrameLeft();
        for (int i = 0; i < windowSize->y - 3; i++)
        {
            moveLastFrameRight();
        }
        documentInfo->frameY += windowSize->y - 3;
    }
    position->current = documentInfo->frameFirstNode;
    position->x = 0;
    position->y = 0;

    move(position->y, position->x);
    print();
}

void saveFileAsFilename(char *filename)
{
    FILE *file = fopen(filename, "w");
    Node *p = head->next;
    while (p != tail->prev)
    {
        fputc((char)p->data, file);
        p = p->next;
    }
    fclose(file);

    for (int i = 0; i < windowSize->x; i++)
    {
        mvprintw(windowSize->y - 1, i, " ");
    }

    mvprintw(windowSize->y - 1, 0, "Save %s successfully.", filename);
}

void save(void)
{
    fileInfo->isFileSaving = true;

    if (fileInfo->isNewFile)
    { // 새 파일인 경우
        char newFilename[100];
        int index = 0;

        // 오른쪽의 메세지
        char rightMessage[100];
        sprintf(rightMessage, "Enter = save | Esc = cancel");
        mvprintw(windowSize->y - 1, windowSize->x - strlen(rightMessage), rightMessage);

        for (int i = 0; i < windowSize->x - strlen(rightMessage); i++)
        {
            mvprintw(windowSize->y - 1, i, " ");
        }
        move(windowSize->y - 1, 0);

        while (1)
        { // 파일이름 받아오기
            char ch = getch();
            if (ch == ENTER)
            { // 파일 이름으로 저장하기
                saveFileAsFilename(newFilename);
                break;
            }
            else if (ch == BACKSPACE)
            { // 파일 이름 backspace
                for (int i = 0; i < index; i++)
                {
                    mvprintw(windowSize->y - 1, i, " ");
                }
                index--;
                newFilename[index] = '\0';
                mvprintw(windowSize->y - 1, 0, newFilename);
            }
            else
            { // 파일 이름 쓰기
                newFilename[index] = ch;
                mvprintw(windowSize->y - 1, 0, newFilename);
                index++;
            }
        }
    }
    else
    { // 기존에 있던 파일을 연 경우
        if (fileInfo->isUpdated)
        { // 변경사항이 있는 경우
            saveFileAsFilename(fileInfo->filename);
        }
        else
        {
            for (int i = 0; i < windowSize->x; i++)
            {
                mvprintw(windowSize->y - 1, i, " ");
            }
            mvprintw(windowSize->y - 1, 0, "There are no changes to the file.");
        }
    }
    fileInfo->isUpdated = false;
    fileInfo->isFileSaving = false;
    move(position->y, position->x);
}

void commonKey(int key)
{

    if (position->x == windowSize->x - 1)
    {
        documentInfo->frameX++;
        // move(++position->y, 1);
        // position->x = 1;
        //    enter();
    }
    else
    {
        move(position->y, ++position->x);
    }
    insert(key);
    print();
}

void readFile(char *filename)
{
    fileInfo->isFileReading = true;
    fileInfo->isNewFile = false;
    // 파일의 확장자 가져오기
    char *dot = strrchr(filename, '.');
    if (dot)
        fileInfo->filetype = dot + 1;
    fileInfo->filename = filename;
    FILE *pFile = fopen(filename, "r");

    int ch;

    bool flag = true;
    do
    {
        ch = fgetc(pFile);
        if (ch == '\n')
        {
            enter();
        }
        else
            commonKey(ch);

        if (flag && documentInfo->lineCount == windowSize->y)
        {
            Node *p = position->current->prev;
            documentInfo->frameLastNode = p;
            flag = false;
        }
    } while (ch != EOF);

    fclose(pFile);
    move(0, 0);
    position->x = 0;
    position->y = 0;
    documentInfo->frameX = 0;
    documentInfo->frameY = 0;
    position->current = head;
    fileInfo->isFileReading = false;
    print();
}

PNode *findWordsInDocument(char *word)
{
    Node *p = head->next;
    PNode *wordListHead = (PNode *)malloc(sizeof(PNode));
    PNode *wordListTail = (PNode *)malloc(sizeof(PNode));

    wordListHead->next = wordListTail;
    wordListHead->prev = wordListTail; // 한 방향으로만 접근 가능하도록 함 (반 원형 연결 리스트?)

    wordListTail->prev = wordListHead;

    int x = 0;
    int y = 0;
    while (p != tail)
    {
        bool valid = true;
        if (p->data == ENTER)
        {
            x = 0;
            y++;
        }
        else
        {
            Node *t = p;
            for (int i = 0; i < strlen(word); i++)
            {
                if (t->data != word[i])
                {
                    valid = false;
                    break;
                }
                t = t->next;
            }
            if (valid)
            {
                PNode *new = (PNode *)malloc(sizeof(PNode));
                new->position = (Position *)malloc(sizeof(Position));
                new->position->current = p;
                new->position->x = x;
                new->position->y = y;

                new->next = wordListTail;
                new->prev = wordListTail->prev;

                wordListTail->prev->next = new;
                wordListTail->prev = new;
            }
            x++;
        }

        p = p->next;
    }
    // PNode* p2 = wordListHead->next;
    // while(p2->next != NULL) {
    //     printw("x%d y%d", p2->position->x, p2->position->y);
    //     p2 = p2->next;
    // }
    return wordListHead;
}

void highlight(PNode *p, char *word, int wordLength)
{
    if (p->position->x + wordLength < windowSize->x - 1)
    {
        documentInfo->frameX = 0;
    }
    else
    {
        documentInfo->frameX = p->position->x + wordLength - (windowSize->x - 1);
    }

    int paddingY = (int)(windowSize->y - 2) / 2; // 페이지 내에서 출력될 단어의 y 위치
    if (p->position->y < windowSize->y - 2)
    {
        documentInfo->frameY = 0;
        documentInfo->frameFirstNode = head;
        documentInfo->frameLastNode = head;
        for (int i = 0; i < windowSize->y - 2; i++)
        {
            moveLastFrameRight();
        }
        paddingY = p->position->y;
    }
    else
    {
        documentInfo->frameY = p->position->y - paddingY;
        documentInfo->frameFirstNode = p->position->current;
        documentInfo->frameLastNode = p->position->current;

        for (int i = 0; i < paddingY + 1; i++)
        {
            moveFirstFrameLeft();
        }
        for (int i = 0; i < windowSize->y - 2 - paddingY; i++)
        {
            moveLastFrameRight();
        }
    }

    print();
    attron(COLOR_PAIR(1));
    mvprintw(paddingY, p->position->x - documentInfo->frameX, "%s", word);
    attroff(COLOR_PAIR(1));
    move(windowSize->y - 1, wordLength - 1);
}

void printFindMessageBar(char *word, int currentResultIndex, int resultCount)
{
    for (int i = 0; i < windowSize->x; i++)
    {
        mvaddch(windowSize->y - 1, i, ' ');
    }
    char rightMessage[100];
    sprintf(rightMessage, "[%d/%d] Arrows = prev/next | Enter = edit | Esc = cancel", currentResultIndex, resultCount);

    mvprintw(windowSize->y - 1, 0, word);
    mvprintw(windowSize->y - 1, windowSize->x - strlen(rightMessage), rightMessage);
    move(windowSize->y - 1, strlen(word));
}

int countFindResult(PNode *wordListHead)
{
    PNode *p = wordListHead;
    int count = 0;
    while (p->next != NULL) // 위의 원형 리스트에서 tail에서는 head로의 연결을 해놓지 않았기 때문에 null로 리스트의 끝을 알수 있음
    {
        count++;
        p = p->next;
    }
    return count - 1;
}

void find(void)
{
    char word[100];
    printFindMessageBar(word, 0, 0);

    // 포기시 원래위치
    Position *tempPosition = position;
    Node *tempFFN = documentInfo->frameFirstNode;
    Node *tempFLN = documentInfo->frameLastNode;
    int tempFX = documentInfo->frameX;
    int tempFY = documentInfo->frameY;

    PNode *wordListHead = (PNode *)malloc(sizeof(PNode));
    PNode *highlightedWord;
    int resultCount = 0;
    int currentResultIndex = 0;
    int wordIndex = 0;


    while (1)
    {
        int ch = getch();
        if (ch == ENTER)
        { // 현재 하이라이트되어있는 위치로 이동
            position->current = highlightedWord->position->current->prev;
            for (int i = 0; i < strlen(word); i++)
            {
                position->current = position->current->next;
            }

            position->x = highlightedWord->position->x + wordIndex - documentInfo->frameX;
            position->y = highlightedWord->position->y - documentInfo->frameY;
            print();
            break;
        }
        else if (ch == BACKSPACE)
        {
            wordIndex--;
            word[wordIndex] = '\0';
            wordListHead = findWordsInDocument(word);
            resultCount = countFindResult(wordListHead);

            if (wordIndex == 0 || resultCount == 0)
            {
                position = tempPosition;
                documentInfo->frameFirstNode = tempFFN;
                documentInfo->frameLastNode = tempFLN;
                documentInfo->frameX = tempFX;
                documentInfo->frameY = tempFY;
                print();
                printFindMessageBar(word, 0, 0);
            }
            else
            {
                highlightedWord = wordListHead->next;
                highlight(highlightedWord, word, wordIndex + 1); // wordLength == wordIndex + 1
                printFindMessageBar(word, currentResultIndex, resultCount);
            }
        }
        else if (ch == KEY_RIGHT)
        {
            if (resultCount == 0)
                continue;
            if (currentResultIndex == resultCount)
            {
                currentResultIndex = 1;
                highlightedWord = wordListHead->next;
            }
            else
            {
                currentResultIndex++;
                highlightedWord = highlightedWord->next;
            }

            highlight(highlightedWord, word, wordIndex + 1); // wordLength == wordIndex + 1
            printFindMessageBar(word, currentResultIndex, resultCount);
        }
        else if (ch == KEY_LEFT)
        {
            if (resultCount == 0)
                continue;
            if (currentResultIndex == 1)
            {
                currentResultIndex = resultCount;
                highlightedWord = wordListHead->prev->prev; // 위에서 언급한 반 원형 연결 리스트? 의 활용
            }
            else
            {
                currentResultIndex--;
                highlightedWord = highlightedWord->prev;
            }

            highlight(highlightedWord, word, wordIndex + 1); // wordLength == wordIndex + 1
            printFindMessageBar(word, currentResultIndex, resultCount);
        }
        else if(ch == ESC) {
            print();
            break;
        }
        else
        {
            word[wordIndex] = ch;
            wordIndex++;

            wordListHead = findWordsInDocument(word);

            resultCount = countFindResult(wordListHead);
            if (wordIndex == 0 || resultCount == 0)
            {
                position = tempPosition;
                documentInfo->frameFirstNode = tempFFN;
                documentInfo->frameLastNode = tempFLN;
                documentInfo->frameX = tempFX;
                documentInfo->frameY = tempFY;
                print();
                printFindMessageBar(word, 0, 0);
            }
            else
            {
                currentResultIndex = 1;
                highlightedWord = wordListHead->next;
                highlight(highlightedWord, word, wordIndex + 1); // wordLength == wordIndex + 1
                printFindMessageBar(word, 1, resultCount);
            }
        }
    }
    free(wordListHead);
}

void quit(void)
{
    if (fileInfo->isUpdated)
    {
        for (int i = 0; i < windowSize->x; i++)
            mvprintw(windowSize->y - 1, i, " ");
        mvprintw(windowSize->y - 1, 0, "Press Ctrl-q again to leave without saving.");
        int ch = getch();
        if (ch != CTRL('q'))
        {
            move(position->y, position->x);
            print();
            return;
        }
    }
    endwin();
    exit(0);
}

struct termios orig_termios;

void disableCtrlFunctions()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON);
    raw.c_lflag &= ~(IEXTEN | ISIG | ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[])
{
    initLinkedList();
    initCurses();
    initWindowSize();
    initDocumentInfo();
    initFileInfo();

    print();
    disableCtrlFunctions();

    if (argv[1] != NULL)
        readFile(argv[1]);

    while (1)
    {
        int key = getch();
        if (key == BACKSPACE)
        {
            fileInfo->isUpdated = true;
            backspace();
        }

        else if (key == ENTER)
        {
            fileInfo->isUpdated = true;
            enter();
        }
        else if (key == KEY_UP)
            arrowUp();
        else if (key == KEY_DOWN)
            arrowDown();
        else if (key == KEY_RIGHT)
            arrowRight();
        else if (key == KEY_LEFT)
            arrowLeft();
        else if (key == KEY_HOME)
            home();
        else if (key == KEY_END)
            end();

        else if (key == KEY_PPAGE)
            pageUp();
        else if (key == KEY_NPAGE)
            pageDown();

        else if (key == CTRL('f'))
            find();
        else if (key == CTRL('s'))
            save();
        else if (key == CTRL('q'))
            quit();
        else
        {
            fileInfo->isUpdated = true;
            commonKey(key);
        }
    }
    return 0;
}
