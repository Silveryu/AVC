#ifndef PREDICTORS_H
#define PREDICTORS_H

#include <vector>
#include <array>

// temporal predictor, predictor[0] has x[n-(len-1)]

enum class PredictorType
{
    InterChannel,
    TemporalOrder1,
    TemporalOrder2,
    TemporalOrder3
};

class Predictor {
    
    public:
        std::vector<int> kernel;
        std::string name;
        const PredictorType type;
        const bool isTemporal;

        Predictor(PredictorType type): 
            type(type),
            isTemporal(type != PredictorType::InterChannel) {
                switch(type){
                    case PredictorType::TemporalOrder1:
                        kernel = std::vector<int>{ 1 };
                        name = "TemporalOrder1";
                        break;
                    case PredictorType::TemporalOrder2:
                        kernel = std::vector<int>{ 2, -1 };
                        name = "TemporalOrder2";
                        break;
                    case PredictorType::TemporalOrder3:
                        kernel = std::vector<int>{ 3, -3, 1 };
                        name = "TemporalOrder3";
                        break;
                    case PredictorType::InterChannel:
                        name = "InterChannel";
                        break;
                    default:
                        std::cerr << "Invalid predictor type" << std::endl;
                        break;
                }
        }
        static const std::array<PredictorType, 4> supportedPredictorTypes;
};

const std::array<PredictorType, 4> Predictor::supportedPredictorTypes { 
    PredictorType::InterChannel,
    PredictorType::TemporalOrder1, 
    PredictorType::TemporalOrder2, 
    PredictorType::TemporalOrder3 
};



#endif