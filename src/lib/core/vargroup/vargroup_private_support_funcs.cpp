#include "vargroup_private_support_funcs.h"
#include "dynaplex/error.h"
#include "picosha2.h"
//only called from vargroup implementation file. To break that file up. 
namespace DynaPlex::VarGroupHelpers {


    int64_t levenshteinDist(const std::string& word1, const std::string& word2) {
        int size1 = word1.size();
        int size2 = word2.size();

        // Create a 2D vector for the dynamic programming table.
        std::vector<std::vector<int64_t>> verif(size1 + 1, std::vector<int64_t>(size2 + 1, 0));

        // Initialization
        for (int i = 0; i <= size1; i++)
            verif[i][0] = i;
        for (int j = 0; j <= size2; j++)
            verif[0][j] = j;

        // Fill the table using a dynamic programming approach.
        for (int i = 1; i <= size1; i++) {
            for (int j = 1; j <= size2; j++) {
                int64_t cost = (word1[i - 1] == word2[j - 1]) ? 0 : 1;
                verif[i][j] = std::min({ verif[i - 1][j] + 1, verif[i][j - 1] + 1, verif[i - 1][j - 1] + cost });
            }
        }

        return verif[size1][size2];
    }



    //Stuff related to hash_json

    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-=";       

    std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
        std::string ret;
        int len = ((in_len + 2) / 3) * 4+1;
        ret.reserve(len);

        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];

            while ((i++ < 3))
                 ret += '_';
            ret += '_';
        }
        return ret;
    }


    int64_t hash_to_int64(const std::string& str) {
        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(str.begin(), str.end(), hash.begin(), hash.end());

        int64_t segment1 = 0;
        int64_t segment2 = 0;
        int64_t segment3 = 0;
        int64_t segment4 = 0;

        for (int i = 0; i < 8; ++i) {
            segment1 = (segment1 << 8) | hash[i];
            segment2 = (segment2 << 8) | hash[i + 8];
            segment3 = (segment3 << 8) | hash[i + 16];
            segment4 = (segment4 << 8) | hash[i + 24];
        }

        return segment1 ^ segment2 ^ segment3 ^ segment4;
    }

    std::string hash_string(const std::string& str) {
        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(str.begin(), str.end(), hash.begin(), hash.end());
        return base64_encode(hash.data(), hash.size());
    }  


    // Function to sort an ordered_json object
    ordered_json sort_json(const ordered_json& j) {
        ordered_json ordered_j;

        // Check if the input is an object
        if (j.is_object()) {
            std::map<std::string, ordered_json> sorted_map;

            // Sort the keys of the object
            for (auto& element : j.items()) {
                sorted_map[element.key()] = element.value();
            }

            // Recursively sort object values
            for (auto& element : sorted_map) {
                ordered_j[element.first] = sort_json(element.second);
            }
        }
        // Check if the input is an array
        else if (j.is_array()) {
            for (auto& element : j) {
                ordered_j.push_back(sort_json(element));
            }
        }
        // If the input is not an object or array, simply assign its value
        else {
            ordered_j = j;
        }

        return ordered_j;
    }

    // Function to hash a sorted ordered_json object
    std::string hash_json_string(const ordered_json& j) {
        // Sort the JSON object
        ordered_json sorted_j = sort_json(j);

        // Serialize the sorted object to a string
        std::string serialized = sorted_j.dump();

        return hash_string(serialized);
    }


    // Function to hash a sorted ordered_json object
    int64_t hash_json_int64(const ordered_json& j) {
        // Sort the JSON object
        ordered_json sorted_j = sort_json(j);

        // Serialize the sorted object to a string
        std::string serialized = sorted_j.dump();

        return hash_to_int64(serialized);
    }

    //stuff related to check_validity

    bool check_homogeneity(const ordered_json& j, const std::string& path) {
        if (j.is_array()) {
            // Check if the array is homogeneous
            ordered_json::value_t array_type;
            bool first = true;

            for (const auto& element : j) {
                if (first) {
                    if (element.is_number_integer() || element.is_number_unsigned()) {
                        array_type = ordered_json::value_t::number_integer;
                    }
                    else {
                        array_type = element.type();
                    }

                    if (element.is_boolean())
                    {
                        throw DynaPlex::Error("Boolean value not allowed in array, at path: " + path);
                    }
                    if (element.is_array())
                    {
                        throw DynaPlex::Error("Direct nesting of array inside array not supported, at path: " + path);
                    }
                    first = false;
                }
                else {
                    ordered_json::value_t element_type;
                    if (element.is_number_integer() || element.is_number_unsigned()) {
                        element_type = ordered_json::value_t::number_integer;
                    }
                    else {
                        element_type = element.type();
                    }

                    if (array_type != element_type) {
                        throw DynaPlex::Error("Inhomogeneous array at path: " + path);
                    }
                }
            }

            // Check if the array elements are either primitive or structured
            int i{ 0 };
            for (const auto& element : j) {
                std::string key = "[" + std::to_string((i++)) + "]";
                std::string child_path = path + key;
                if (!check_homogeneity(element, child_path)) {
                    return false;
                }
            }
        }
        else if (j.is_object()) {
            for (const auto& [key, value] : j.items()) {
                std::string child_path = path.empty() ? key : (path + "." + key);
                if (!check_homogeneity(value, child_path)) {
                    return false;
                }
            }
        }
        else
        {
            if (j.is_binary())
            {
                throw DynaPlex::Error("Binary values not allowed");
            }
            if (j.is_discarded())
            {
                throw DynaPlex::Error("Discarded values not allowed");
            }
        }

        return true;
    }

    bool check_validity(const ordered_json& j)
    {
        if (!j.is_object())
        {
            throw DynaPlex::Error("Root node is not an object/dict.");
        }
        return check_homogeneity(j, "");
    }

    void SortOrderedJson(ordered_json& oj) {
        std::vector<nlohmann::ordered_json::iterator> iterators;
        for (auto it = oj.begin(); it != oj.end(); ++it) {
            iterators.push_back(it);
        }

        // Sort iterators based on their keys
        std::sort(iterators.begin(), iterators.end(), [](const auto& a, const auto& b) {
            return a.key() < b.key();
            });

        nlohmann::ordered_json sorted;
        for (const auto& it : iterators) {
            sorted[it.key()] = *it;
        }

        oj.swap(sorted);
    }

 
}  // namespace DynaPlex
