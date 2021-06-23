#pragma once

#include "../utils/Log.h"

#include <memory>
#include <vector>

// Based on https://github.com/ekg/intervaltree
namespace EGL3::Service {
    template <class Scalar, typename Value>
    class Interval {
    public:
        Scalar start;
        Scalar stop;
        Value value;
        Interval(const Scalar& s, const Scalar& e, const Value& v) :
            start(std::min(s, e)),
            stop(std::max(s, e)),
            value(v)
        {}
    };

    template<bool UseStop>
    struct IntervalCmp {
        template <class ScalarT, class ValueT>
        bool operator()(const Interval<ScalarT, ValueT>& a, const Interval<ScalarT, ValueT>& b) {
            if constexpr (!UseStop) {
                return a.start < b.start;
            }
            else {
                return a.stop < b.stop;
            }
        }
    };

    using IntervalStartCmp = IntervalCmp<false>;
    using IntervalStopCmp = IntervalCmp<true>;

    template <class ScalarT, class ValueT>
    class IntervalTree {
    public:
        using IntervalT = Interval<ScalarT, ValueT>;
        using IntervalList = std::vector<IntervalT>;

        IntervalTree() = default;
        ~IntervalTree() = default;

        IntervalTree(const IntervalTree&) = delete;
        IntervalTree& operator=(const IntervalTree&) = delete;

        IntervalTree(IntervalTree&&) = default;
        IntervalTree& operator=(IntervalTree&&) = default;

        IntervalTree(IntervalList&& Input, size_t Depth = 16, size_t BucketMin = 64, size_t BucketMax = 512, ScalarT ExtentL = 0, ScalarT ExtentR = 0) : IntervalTree()
        {
            --Depth;

            if (!Input.empty()) {
                Center = (std::min_element(Input.begin(), Input.end(), IntervalStartCmp{})->start +
                          std::max_element(Input.begin(), Input.end(), IntervalStopCmp{})->stop) / 2;
            }

            // Ensure input is sorted
            if (ExtentL == 0 && ExtentR == 0) {
                std::sort(Input.begin(), Input.end(), IntervalStartCmp{});
            }
            else {
                EGL3_VERIFY(std::is_sorted(Input.begin(), Input.end(), IntervalStartCmp{}), "Input is not sorted");
            }

            if (Depth == 0 || (Input.size() < BucketMin && Input.size() < BucketMax)) {
                std::sort(Input.begin(), Input.end(), IntervalStartCmp{});
                Intervals = std::move(Input);
                EGL3_VERIFY(IsValid().first, "New interval list is not valid");
                return;
            }
            else {
                ScalarT LeftP, RightP;
                if (ExtentL || ExtentR) {
                    LeftP = ExtentL;
                    RightP = ExtentR;
                }
                else {
                    LeftP = Input.front().start;
                    RightP = std::max_element(Input.begin(), Input.end(), IntervalStopCmp{})->stop;
                }

                IntervalList Lefts;
                IntervalList Rights;

                for (auto&& Int : Input) {
                    if (Int.stop < Center) {
                        Lefts.emplace_back(std::move(Int));
                    }
                    else if (Int.start > Center) {
                        Rights.emplace_back(std::move(Int));
                    }
                    else {
                        EGL3_VERIFY(Int.start <= Center && Center <= Int.stop, "Interval is not between the given center");
                        Intervals.emplace_back(std::move(Int));
                    }
                }

                if (!Lefts.empty()) {
                    Left.reset(new IntervalTree(std::move(Lefts), Depth, BucketMin, BucketMax, LeftP, Center));
                }
                if (!Rights.empty()) {
                    Right.reset(new IntervalTree(std::move(Rights), Depth, BucketMin, BucketMax, Center, RightP));
                }
            }
            EGL3_VERIFY(IsValid().first, "New interval list is not valid");
        }

        // Call f on all intervals near the range [start, stop]:
        template <class UnaryFunction>
        void VisitNear(const ScalarT& Start, const ScalarT& Stop, UnaryFunction f) const {
            if (!Intervals.empty() && !(Stop < Intervals.front().start)) {
                std::for_each(Intervals.begin(), Intervals.end(), f);
            }
            if (Left && Start <= Center) {
                Left->VisitNear(Start, Stop, f);
            }
            if (Right && Stop >= Center) {
                Right->VisitNear(Start, Stop, f);
            }
        }

        // Call f on all intervals crossing pos
        template <class UnaryFunction>
        void VisitOverlapping(const ScalarT& Pos, UnaryFunction f) const {
            VisitOverlapping(Pos, Pos, f);
        }

        // Call f on all intervals overlapping [start, stop]
        template <class UnaryFunction>
        void VisitOverlapping(const ScalarT& Start, const ScalarT& Stop, UnaryFunction f) const {
            VisitNear(Start, Stop, [&](const auto& Interval) {
                if (Interval.stop >= Start && Interval.start <= Stop) {
                    // Only apply f if overlapping
                    f(Interval);
                }
            });
        }

        // Call f on all intervals contained within [start, stop]
        template <class UnaryFunction>
        void VisitContained(const ScalarT& Start, const ScalarT& Stop, UnaryFunction f) const {
            VisitNear(Start, Stop, [&](const auto& Interval) {
                if (Start <= Interval.start && Interval.stop <= Stop) {
                    f(Interval);
                }
            });
        }

        IntervalList FindOverlapping(const ScalarT& Start, const ScalarT& Stop) const {
            IntervalList Ret;
            VisitOverlapping(Start, Stop, [&](const auto& Interval) { Ret.emplace_back(Interval); });
            return Ret;
        }

        IntervalList FindContained(const ScalarT& Start, const ScalarT& Stop) const {
            IntervalList Ret;
            VisitContained(Start, Stop, [&](const auto& Interval) { Ret.emplace_back(Interval); });
            return Ret;
        }

        bool IsEmpty() const {
            return (Left && Left->empty()) || Intervals.empty() || (Right && Right->empty());
        }

        // Check all constraints.
        // If first is false, second is invalid.
        std::pair<bool, std::pair<ScalarT, ScalarT>> IsValid() const {
            const auto minmaxStart = std::minmax_element(Intervals.begin(), Intervals.end(), IntervalStartCmp{});
            const auto minmaxStop = std::minmax_element(Intervals.begin(), Intervals.end(), IntervalStopCmp{});

            std::pair<bool, std::pair<ScalarT, ScalarT>> result = { true, { std::numeric_limits<ScalarT>::max(),
                                                                         std::numeric_limits<ScalarT>::min() } };
            if (!Intervals.empty()) {
                result.second.first = std::min(result.second.first, minmaxStart.first->start);
                result.second.second = std::min(result.second.second, minmaxStop.second->stop);
            }
            if (Left) {
                auto valid = Left->IsValid();
                result.first &= valid.first;
                result.second.first = std::min(result.second.first, valid.second.first);
                result.second.second = std::min(result.second.second, valid.second.second);
                if (!result.first) { return result; }
                if (valid.second.second >= Center) {
                    result.first = false;
                    return result;
                }
            }
            if (Right) {
                auto valid = Right->IsValid();
                result.first &= valid.first;
                result.second.first = std::min(result.second.first, valid.second.first);
                result.second.second = std::min(result.second.second, valid.second.second);
                if (!result.first) { return result; }
                if (valid.second.first <= Center) {
                    result.first = false;
                    return result;
                }
            }
            if (!std::is_sorted(Intervals.begin(), Intervals.end(), IntervalStartCmp{})) {
                result.first = false;
            }
            return result;
        }

    private:
        IntervalList Intervals;
        std::unique_ptr<IntervalTree> Left;
        std::unique_ptr<IntervalTree> Right;
        ScalarT Center = 0;
    };
}