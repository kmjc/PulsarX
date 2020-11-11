/*
 * equalize.h
 *
 *  Created on: May 5, 2020
 *      Author: ypmen
 */

#ifndef EQUALIZE_H_
#define EQUALIZE_H_

#include "databuffer.h"

class Equalize : public DataBuffer<float>
{
public:
    Equalize();
    Equalize(const Equalize &equalize);
    Equalize & operator=(const Equalize &equalize);
    ~Equalize();
    void prepare(DataBuffer<float> &databuffer);
    void run(DataBuffer<float> &databuffer);
private:
    vector<double> chmean;
    vector<double> chstd;
};

#endif /* EQUALIZE_H_ */