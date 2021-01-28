#pragma once

#include "ExpressionError.h"
#include "ExpressionToken.h"
#include "OperatorFunctionId.h"

#include <functional>

namespace EGL3::Utils::StringEx {
    template<class T>
    class JumpTable {
    public:
        using UnaryFunction = std::function<ExpressionError(const std::any&, const T*, std::any&)>;
        using BinaryFunction = std::function<ExpressionError(const std::any&, const std::any&, const T*, std::any&)>;
        using ShortCircuit = std::function<bool(const std::any&, const T*)>;

        ExpressionError ExecPreUnary(const ExpressionToken& Operator, const ExpressionToken& R, const T* Ctx, std::any& Output) const {
            auto Itr = PreUnaryOps.find(OperatorFunctionId{ Operator.Node.type(), typeid(void), R.Node.type() });
            if (Itr != PreUnaryOps.end()) {
                return Itr->second(R.Node, Ctx, Output);
            }

            return ExpressionError("Preunary operator cannot operate on such types");
        }
        
        ExpressionError ExecPostUnary(const ExpressionToken& Operator, const ExpressionToken& L, const T* Ctx, std::any& Output) const {
            auto Itr = PostUnaryOps.find(OperatorFunctionId{ Operator.Node.type(), L.Node.type(), typeid(void) });
            if (Itr != PostUnaryOps.end()) {
                return Itr->second(L.Node, Ctx, Output);
            }

            return ExpressionError("Postunary operator cannot operate on such types");
        }

        ExpressionError ExecBinary(const ExpressionToken& Operator, const ExpressionToken& L, const ExpressionToken& R, const T* Ctx, std::any& Output) const {
            auto Itr = BinaryOps.find(OperatorFunctionId{ Operator.Node.type(), L.Node.type(), R.Node.type() });
            if (Itr != BinaryOps.end()) {
                return Itr->second(L.Node, R.Node, Ctx, Output);
            }

            return ExpressionError("Binary operator cannot operate on such types");
        }

        bool ShouldShortCircuit(const ExpressionToken& Operator, const ExpressionToken& L, const T* Ctx) const {
            auto Itr = BinaryShortCircuits.find(OperatorFunctionId{ Operator.Node.type(), L.Node.type(), typeid(void) });
            if (Itr != BinaryShortCircuits.end()) {
                return Itr->second(L.Node, Ctx);
            }

            return false;
        }

        // T(T)
        template<typename FuncType, typename InT>
        static UnaryFunction WrapUnaryFunction() {
            return [](const std::any& In, const T* Ctx, std::any& Out) -> ExpressionError {
                Out = FuncType{}(std::any_cast<InT>(In));
                return ExpressionError();
            };
        }

        // T(T, Ctx)
        template<typename FuncType, typename InT>
        static UnaryFunction WrapUnaryFunctionCtx() {
            return [](const std::any& In, const T* Ctx, std::any& Out) -> ExpressionError {
                Out = FuncType{}(std::any_cast<InT>(In), Ctx);
                return ExpressionError();
            };
        }

        // Error(T, Ctx, Out)
        template<typename FuncType, typename InT>
        static UnaryFunction WrapUnaryFunctionFull() {
            return [](const std::any& In, const T* Ctx, std::any& Out) -> ExpressionError {
                return FuncType{}(std::any_cast<InT>(In), Ctx, Out);
            };
        }

        // T(T, T)
        template<typename FuncType, typename InLT, typename InRT>
        static BinaryFunction WrapBinaryFunction() {
            return [](const std::any& InL, const std::any& InR, const T* Ctx, std::any& Out) -> ExpressionError {
                Out = FuncType{}(std::any_cast<InLT>(InL), std::any_cast<InRT>(InR));
                return ExpressionError();
            };
        }

        // T(T, T, Ctx)
        template<typename FuncType, typename InLT, typename InRT>
        static BinaryFunction WrapBinaryFunctionCtx() {
            return [](const std::any& InL, const std::any& InR, const T* Ctx, std::any& Out) -> ExpressionError {
                Out = FuncType{}(std::any_cast<InLT>(InL), std::any_cast<InRT>(InR), Ctx);
                return ExpressionError();
            };
        }

        // Error(T, T, Ctx, Out)
        template<typename FuncType, typename InLT, typename InRT>
        static BinaryFunction WrapBinaryFunctionFull() {
            return [](const std::any& InL, const std::any& InR, const T* Ctx, std::any& Out) -> ExpressionError {
                return FuncType{}(std::any_cast<InLT>(InL), std::any_cast<InRT>(InR), Ctx, Out);
            };
        }

        template<typename OperatorType, typename OperandType>
        void MapPreUnary() {
            PreUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunction<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapPreUnaryCtx() {
            PreUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunctionCtx<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapPreUnaryFull() {
            PreUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunctionFull<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapPostUnary() {
            PostUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunction<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapPostUnaryCtx() {
            PostUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunctionCtx<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapPostUnaryFull() {
            PostUnaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(void), typeid(OperandType) }, WrapUnaryFunctionFull<OperatorType, OperandType>());
        }

        template<typename OperatorType, typename LeftOperandType, typename RightOperandType = LeftOperandType>
        void MapBinary() {
            BinaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(LeftOperandType), typeid(RightOperandType) }, WrapBinaryFunction<OperatorType, LeftOperandType, RightOperandType>());
        }

        template<typename OperatorType, typename LeftOperandType, typename RightOperandType = LeftOperandType>
        void MapBinaryCtx() {
            BinaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(LeftOperandType), typeid(RightOperandType) }, WrapBinaryFunctionCtx<OperatorType, LeftOperandType, RightOperandType>());
        }

        template<typename OperatorType, typename LeftOperandType, typename RightOperandType = LeftOperandType>
        void MapBinaryFull() {
            BinaryOps.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(LeftOperandType), typeid(RightOperandType) }, WrapBinaryFunctionFull<OperatorType, LeftOperandType, RightOperandType>());
        }

        template<typename OperatorType, typename OperandType>
        void MapShortCircuit() {
            BinaryShortCircuits.emplace(OperatorFunctionId{ typeid(OperatorType), typeid(OperandType), typeid(void) }, WrapShortCircuitFunction(OperatorType{}));
        }

    private:
        std::unordered_map<OperatorFunctionId, UnaryFunction> PreUnaryOps;
        std::unordered_map<OperatorFunctionId, UnaryFunction> PostUnaryOps;
        std::unordered_map<OperatorFunctionId, BinaryFunction> BinaryOps;
        std::unordered_map<OperatorFunctionId, ShortCircuit> BinaryShortCircuits;
    };
}