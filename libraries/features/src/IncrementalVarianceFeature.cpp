////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     IncrementalVarianceFeature.cpp (features)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "IncrementalVarianceFeature.h"
#include "MeanFeature.h"
#include "Feature.h"
#include "StringUtil.h"

// layers
#include "ShiftRegisterLayer.h"
#include "ConstantLayer.h"
#include "AccumulatorLayer.h"
#include "BinaryOperationLayer.h"
#include "Sum.h"
#include "CoordinateListTools.h"

// utilities
#include "Exception.h"

// stl
#include <cassert>
#include <cmath>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace features
{
    //
    // IncrementalVarianceFeature
    //

    IncrementalVarianceFeature::IncrementalVarianceFeature(Feature* inputFeature, size_t windowSize)  : BufferedFeature({inputFeature}, windowSize) 
    {}

    IncrementalVarianceFeature::IncrementalVarianceFeature(const std::string& id, Feature* inputFeature, size_t windowSize)  : BufferedFeature(id, {inputFeature}, windowSize) 
    {}
    
    std::vector<double> IncrementalVarianceFeature::ComputeOutput() const
    {
        assert(_inputFeatures.size() == 1);
        const auto& inputData = _inputFeatures[0]->GetOutput();
        auto inputDimension = inputData.size();    
        if(inputDimension == 0) 
        {
            throw utilities::Exception(utilities::ExceptionErrorCodes::invalidArgument, "Invalid input of size zero");
            return inputData;
        }
        _outputDimension = inputDimension;
        auto windowSize = GetWindowSize();
        
        // get the oldest sample
        auto oldData = GetDelayedSamples(windowSize-1);
        oldData.resize(inputDimension);
        _runningSum.resize(inputDimension);
        _runningSumSq.resize(inputDimension);
         
        UpdateRowSamples(inputData);

        // update the running sum and output
        std::vector<double> result(inputDimension);
        for(size_t index = 0; index < inputDimension; ++index)
        {
            auto newVal = inputData[index];
            auto oldVal = oldData[index];
            _runningSum[index] += (newVal - oldVal);
            _runningSumSq[index] += (newVal*newVal - oldVal*oldVal);

            result[index] = (_runningSumSq[index] - (_runningSum[index]*_runningSum[index] / windowSize)) / windowSize;
        }

        return result;
    }

    layers::CoordinateList IncrementalVarianceFeature::AddToModel(layers::Model& model, const std::unordered_map<const Feature*, layers::CoordinateList>& featureOutputs) const
    {
        auto inputIterator = featureOutputs.find(_inputFeatures[0]);
        if (inputIterator == featureOutputs.end())
        {
            throw utilities::Exception(utilities::ExceptionErrorCodes::illegalState, "Couldn't find input feature");
        }
       
        auto inputCoordinates = inputIterator->second;
        auto inputDimension = inputCoordinates.Size();
        auto windowSize = GetWindowSize();

        // Make a layer holding the constant `windowSize`, broadcast to be wide enough to apply to all input dimensions
        auto divisor = model.EmplaceLayer<layers::ConstantLayer>(std::vector<double>{(double)windowSize});        
        auto divisorVector = layers::RepeatCoordinates(divisor, inputDimension);

        // Make a buffer that will hold `windowSize` samples
        auto bufferOutput = model.EmplaceLayer<layers::ShiftRegisterLayer>(inputCoordinates, windowSize+1);
        auto shiftRegisterLayer = dynamic_cast<const layers::ShiftRegisterLayer&>(model.GetLastLayer());

        // Compute running sum by subtracting oldest value and adding newest
        auto oldestSample = shiftRegisterLayer.GetDelayedOutputCoordinates(bufferOutput, windowSize);
        auto diff = model.EmplaceLayer<layers::BinaryOperationLayer>(inputCoordinates, oldestSample, layers::BinaryOperationLayer::OperationType::subtract);
        auto runningSum = model.EmplaceLayer<layers::AccumulatorLayer>(diff);

        // Square the sum of inputs and divide by window size
        auto squaredSum = model.EmplaceLayer<layers::BinaryOperationLayer>(runningSum, runningSum, layers::BinaryOperationLayer::OperationType::multiply);
        auto normSquaredSum = model.EmplaceLayer<layers::BinaryOperationLayer>(squaredSum, divisorVector, layers::BinaryOperationLayer::OperationType::divide);
        
        // Accumulate running sum of squared samples 
        auto newValueSquared = model.EmplaceLayer<layers::BinaryOperationLayer>(inputCoordinates, inputCoordinates, layers::BinaryOperationLayer::OperationType::multiply);
        auto oldValueSquared = model.EmplaceLayer<layers::BinaryOperationLayer>(oldestSample, oldestSample, layers::BinaryOperationLayer::OperationType::multiply);
        auto diffSquared = model.EmplaceLayer<layers::BinaryOperationLayer>(newValueSquared, oldValueSquared, layers::BinaryOperationLayer::OperationType::subtract);
        auto runningSquaredSum = model.EmplaceLayer<layers::AccumulatorLayer>(diffSquared);

        // Compute variance from above values (var = (sum(x^2) - (sum(x)^2 / N)) / N )
        auto varianceTimesN = model.EmplaceLayer<layers::BinaryOperationLayer>(runningSquaredSum, normSquaredSum, layers::BinaryOperationLayer::OperationType::subtract);
        auto variance = model.EmplaceLayer<layers::BinaryOperationLayer>(varianceTimesN, divisorVector, layers::BinaryOperationLayer::OperationType::divide);
        return variance;
    }

    std::unique_ptr<Feature> IncrementalVarianceFeature::Create(std::vector<std::string> params, Feature::FeatureMap& previousFeatures)
    {
        assert(params.size() == 4);
        auto featureId = params[0];
        Feature* inputFeature = previousFeatures[params[2]];
        uint64_t windowSize = ParseInt(params[3]);

        if (inputFeature == nullptr)
        {
            throw utilities::Exception(utilities::ExceptionErrorCodes::badStringFormat, "Error deserializing feature description: unknown input feature " + params[2]);
        }
        return std::make_unique<IncrementalVarianceFeature>(featureId, inputFeature, windowSize);
    }
}
