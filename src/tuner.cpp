#include "lizard.h"
#include <ctime>
#include <stdlib.h>

#ifdef USE_TUNING

double k_const = 1.335;

void cTuner::LoadAll() {

    char line[256];
    char* pos;
    allCnt = 0;

    FILE* epdFile = NULL;
    epdFile = fopen("quiet.epd", "r");
    printf("reading epdFile 'quiet.epd' (%s)\n", epdFile == NULL ? "failure" : "success");

    while (fgets(line, sizeof(line), epdFile)) {    // read positions line by line

        while ((pos = strpbrk(line, "\r\n"))) *pos = '\0'; // cleanup

        allEpd[allCnt] = line;
        allCnt++;
        if (allCnt % 1000000 == 0)
            printf("%d positions loaded\n", allCnt);
    }

    fclose(epdFile);
    
    epdFile = fopen("new.epd", "r");
    printf("reading epdFile 'new.epd' (%s)\n", epdFile == NULL ? "failure" : "success");

    while (fgets(line, sizeof(line), epdFile)) {    // read positions line by line

        while ((pos = strpbrk(line, "\r\n"))) *pos = '\0'; // cleanup

        allEpd[allCnt] = line;
        allCnt++;
        if (allCnt % 1000000 == 0)
            printf("%d positions loaded\n", allCnt);
    }

    fclose(epdFile);

    /*
    epdFile = fopen("filtered.epd", "r");
    printf("reading epdFile 'filtered.epd' (%s)\n", epdFile == NULL ? "failure" : "success");

    while (fgets(line, sizeof(line), epdFile)) {    // read positions line by line

        while ((pos = strpbrk(line, "\r\n"))) *pos = '\0'; // cleanup

        allEpd[allCnt] = line;
        allCnt++;
        if (allCnt % 1000000 == 0)
            printf("%d positions loaded\n", allCnt);
    }
    

    fclose(epdFile);
    */

    printf("%d positions loaded\n", allCnt);

}

void cTuner::InitBatch(int filterValue) 
{
    char line[256];
    std::string posString;
    int readCnt = 0;
    cnt10 = 0;
    cnt01 = 0;
    cnt05 = 0;
    srand(time(0));

    for (int i = 0; i < allCnt; i++) {

        // filtering to cut the batch to size

        int stepOver = rand() % 100000;
        if (stepOver > filterValue)
            continue;

        // sorting strings by game result

        posString = allEpd[i];
        readCnt++;

        if (posString.find("1/2-1/2") != std::string::npos) {
            epd05[cnt05] = posString;
            cnt05++;
        }
        else if (posString.find("1-0") != std::string::npos) {
            epd10[cnt10] = posString;
            cnt10++;
        }
        else if (posString.find("0-1") != std::string::npos) {
            epd01[cnt01] = posString;
            cnt01++;
        }
    }

    printf("%d Total positions loaded\n", readCnt);
}

double cTuner::TexelFit(Position* p, int* pv) {

    int score = 0;
    double sigmoid = 0.0;
    double sum = 0.0;
    int iteration = 0;

    double result = 1;

    for (int i = 0; i < cnt10; ++i) {
        char* cstr = new char[epd10[i].length() + 1];
        strcpy(cstr, epd10[i].c_str());
        SetPosition(p, cstr);
        delete[] cstr;
        if (InCheck(p)) continue;
        //score = Quiesce(p, 0, -INF, INF, pv);
        score = Evaluate(p);
        if (p->side == Black) score = -score;
        sigmoid = TexelSigmoid(score, k_const);
        sum += ((result - sigmoid) * (result - sigmoid));
        iteration++;
    }

    result = 0;

    for (int i = 0; i < cnt01; ++i) {
        char* cstr = new char[epd01[i].length() + 1];
        strcpy(cstr, epd01[i].c_str());
        SetPosition(p, cstr);
        delete[] cstr;
        if (InCheck(p)) continue;
        //score = Quiesce(p, 0, -INF, INF, pv);
        score = Evaluate(p);
        if (p->side == Black) score = -score;
        sigmoid = TexelSigmoid(score, k_const);
        sum += ((result - sigmoid) * (result - sigmoid));
        iteration++;
    }

    result = 0.5;

    for (int i = 0; i < cnt05; ++i) {
        char* cstr = new char[epd05[i].length() + 1];
        strcpy(cstr, epd05[i].c_str());
        SetPosition(p, cstr);
        delete[] cstr;
        if (InCheck(p)) continue;
        //score = Quiesce(p, 0, -INF, INF, pv);
        score = Evaluate(p);
        if (p->side == Black) score = -score;
        sigmoid = TexelSigmoid(score, k_const);
        sum += ((result - sigmoid) * (result - sigmoid));
        iteration++;
    }

    //return (1.0 / iteration) * sum;
    return 1000 * ((1.0 / iteration) * sum);
}

double cTuner::TexelSigmoid(int score, double k) {

    double exp = -(k * ((double)score) / 400.0);
    return 1.0 / (1.0 + pow(10.0, exp));
}

const int symSq[64] = {
    H1, G1, F1, E1, D1, C1, B1, A1,
    H2, G2, F2, E2, D2, C2, B2, A2,
    H3, G3, F3, E3, D3, C3, B3, A3,
    H4, G4, F4, E4, D4, C4, B4, A4,
    H5, G5, F5, E5, D5, C5, B5, A5,
    H6, G6, F6, E6, D6, C6, B6, A6,
    H7, G7, F7, E7, D7, C7, B7, A7,
    H8, G8, F8, E8, D8, C8, B8, A8
};

#endif