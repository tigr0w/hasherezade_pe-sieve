#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include "../utils/format_util.h"

#define IS_ENDLINE(c) (c == 0x0A || c == 0xD)
#define IS_PRINTABLE(c) ((c >= 0x20 && c < 0x7f) || IS_ENDLINE(c))

#define SCAN_STRINGS

namespace pesieve {

    namespace stats {

        // Shannon's Entropy calculation based on: https://stackoverflow.com/questions/20965960/shannon-entropy
        template <typename T>
        double calcShannonEntropy(std::map<T, size_t>& histogram, size_t totalSize)
        {
            if (!totalSize) return 0;
            double entropy = 0;
            for (auto it = histogram.begin(); it != histogram.end(); ++it) {
                double p_x = (double)it->second / totalSize;
                if (p_x > 0) entropy -= p_x * log(p_x) / log(2);
            }
            return entropy;
        }

        template <typename T>
        std::string hexdumpValue(const BYTE* in_buf, const size_t max_size)
        {
            std::stringstream ss;
            for (size_t i = 0; i < max_size; i++) {
                ss << "\\x" << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)in_buf[i];
            }
            return ss.str();
        }

        // return the most frequent value
        template <typename T>
        T getMostFrequentValue(IN std::map<size_t, std::set< T >> frequencies)
        {
            auto itr = frequencies.rbegin();
            if (itr == frequencies.rend()) {
                return 0;
            }
            auto setItr = itr->second.begin();
            T mVal = *setItr;
            return mVal;
        }

        // return the number of occurrencies
        template <typename T>
        size_t getMostFrequentValues(IN std::map<size_t, std::set< T >> frequencies, OUT std::set<T> &values)
        {
            auto itr = frequencies.rbegin();
            if (itr == frequencies.rend()) {
                return 0;
            }

            // find the highest frequency:
            size_t mFreq = itr->first;
            values.insert(itr->second.begin(), itr->second.end());
            return mFreq;
        }

        template <typename T>
        bool isAllPrintable(IN std::map<T, size_t>& histogram)
        {
            if (!histogram.size()) return false;

            bool is_printable = true;
            for (auto itr = histogram.begin(); itr != histogram.end(); ++itr) {
                T val = itr->first;
                if (val && !IS_PRINTABLE(val)) {
                    is_printable = false;
                    break;
                }
            }
            return is_printable;
        }

        //----

        // defines what type of stats should be collected
        class StatsSettings
        {
        public:
            StatsSettings()
            {
            }

            // Copy constructor
            StatsSettings(const StatsSettings& p1)
                : searchedStrings(p1.searchedStrings)
            {
            }

            std::string hasSarchedSubstring(std::string& lastStr)
            {
                for (auto itr = searchedStrings.begin(); itr != searchedStrings.end(); ++itr) {
                    const std::string s = *itr;
                    if (lastStr.find(s) != std::string::npos && s.length()) {
                        //std::cout << "[+] KEY for string: " << lastStr << " found: " << s << "\n";
                        return s; // the current string contains searched string
                    }
                }
                //std::cout << "[-] KEY for string: " << lastStr << " NOT found!\n";
                return "";
            }

            std::set<std::string> searchedStrings;
        };

        //----

        template <typename T>
        struct ChunkStats {
            //
            ChunkStats() 
                : size(0), offset(0), entropy(0), longestStr(0), prevVal(0),
                stringsCount(0), cleanStringsCount(0)
            {
            }

            ChunkStats(size_t _offset, size_t _size)
                : size(_size), offset(_offset), entropy(0), longestStr(0), prevVal(0),
                stringsCount(0), cleanStringsCount(0)
            {
            }

            // Copy constructor
            ChunkStats(const ChunkStats& p1)
                : size(p1.size), offset(p1.offset), 
                entropy(p1.entropy), longestStr(p1.longestStr), lastStr(p1.lastStr), prevVal(p1.prevVal), 
                stringsCount(p1.stringsCount), cleanStringsCount(p1.cleanStringsCount)
                histogram(p1.histogram),
                frequencies(p1.frequencies), 
                settings(p1.settings), foundStrings(p1.foundStrings)

#ifdef _KEEP_STR
                , allStrings(p1.allStrings)
#endif //_KEEP_STR
            {
            }

            void fillSettings(StatsSettings &_settings)
            {
                settings = _settings;
            }

            void appendVal(T val)
            {
#ifdef SCAN_STRINGS
                const bool isPrint = IS_PRINTABLE(val);
                if (isPrint){
                    lastStr += char(val);
                }
                else {
                    const bool isClean = (val == 0) ? true : false; //terminated cleanly?
                    finishLastStr(isClean);
                    lastStr.clear();
                }

#endif // SCAN_STRINGS
                size++;
                histogram[val]++;
                prevVal = val;
            }

            void finishLastStr(bool isClean)
            {
                if (lastStr.length() < 2) {
                    return;
                }
                stringsCount++;
                if (isClean) cleanStringsCount++;

                std::string key = settings.hasSarchedSubstring(lastStr);
                if (key.length()) {
                    foundStrings[key]++; // the current string contains searched string
                }

#ifdef _KEEP_STR
                allStrings.push_back(lastStr);
#endif //_KEEP_STR
                //std::cout << "-----> lastStr:" << lastStr << "\n";
                if (lastStr.length() > longestStr) {
                    longestStr = lastStr.length();
                }
                lastStr.clear();
            }

            const virtual void fieldsToJSON(std::stringstream& outs, size_t level)
            {
                OUT_PADDED(outs, level, "\"offset\" : ");
                outs << std::hex << "\"" << offset << "\"";
                outs << ",\n";
                OUT_PADDED(outs, level, "\"size\" : ");
                outs << std::hex << "\"" << size << "\"";
                outs << ",\n";
                OUT_PADDED(outs, level, "\"charset_size\" : ");
                outs << std::dec << histogram.size();
                
                std::set<T> values;
                size_t freq = getMostFrequentValues<T>(frequencies, values);
                if (freq && values.size()) {
                    outs << ",\n";
                    OUT_PADDED(outs, level, "\"most_freq_vals\" : ");
                    outs << std::hex << "\"";
                    for (auto itr = values.begin(); itr != values.end(); ++itr) {
                        T mVal = *itr;
                        outs << hexdumpValue<BYTE>((BYTE*)&mVal, sizeof(T));
                    }
                    outs << "\"";
                }
                outs << ",\n";
                OUT_PADDED(outs, level, "\"entropy\" : ");
                outs << std::dec << entropy;
            }

            void summarize()
            {
                entropy = calcShannonEntropy(histogram, size);
                finishLastStr(true);

                for (auto itr = histogram.begin(); itr != histogram.end(); ++itr) {
                    const size_t count = itr->second;
                    const  T val = itr->first;
                    frequencies[count].insert(val);
                }
            }

            double entropy;
            size_t size;
            size_t offset;

            T prevVal;
            size_t longestStr; // the longest ASCII string in the chunk

            std::string lastStr;
            size_t stringsCount;
            size_t cleanStringsCount;
            std::map<T, size_t> histogram;
            std::map<size_t, std::set< T >>  frequencies;
            StatsSettings settings;

            std::map<std::string, size_t> foundStrings;
#ifdef _KEEP_STR
            std::vector< std::string > allStrings;
#endif
        };

        template <typename T>
        struct AreaStats {
            AreaStats()
            {
            }

            // Copy constructor
            AreaStats(const AreaStats& p1)
                : currArea(p1.currArea)
            {
            }

            void fillSettings(StatsSettings& _settings)
            {
                currArea.fillSettings(_settings);
            }

            const virtual bool toJSON(std::stringstream& outs, size_t level)
            {
                OUT_PADDED(outs, level, "\"stats\" : {\n");
                fieldsToJSON(outs, level + 1);
                outs << "\n";
                OUT_PADDED(outs, level, "}");
                return true;
            }

            const virtual void fieldsToJSON(std::stringstream& outs, size_t level)
            {
                OUT_PADDED(outs, level, "\"full_area\" : {\n");
                currArea.fieldsToJSON(outs, level + 1);
                outs << "\n";
                OUT_PADDED(outs, level, "}");
            }

            bool isFilled() const
            {
                return (currArea.size != 0) ? true : false;
            }

            void summarize()
            {
                currArea.summarize();
            }

            ChunkStats<T> currArea; // stats from the whole area

        };


        template <typename T>
        class AreaStatsCalculator {
        public:
            AreaStatsCalculator(T _data[], size_t _elements)
                :data(_data), elements(_elements)
            {
            }

            bool fill(AreaStats<T> &stats, StatsSettings &settings)
            {
                if (!data || !elements) return false;

                stats.fillSettings(settings);

                T lastVal = 0;
                for (size_t dataIndex = 0; dataIndex < elements; ++dataIndex) {
                    const T val = data[dataIndex];
                    stats.currArea.appendVal(val);
                }
                stats.summarize();
                return true;
            }

        private:
            T *data;
            size_t  elements;
        };

    }; // namespace util

}; //namespace pesieve

