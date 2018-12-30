//
// Created by kamil on 19.10.18.
//

#ifndef SBD_1_RECORD_HH
#define SBD_1_RECORD_HH


#include <cstdint>
#include <array>
#include <ostream>
#include <random>


constexpr const int GRADES_NUMBER = 3;

/*
 * Record contains (uint64_t):
 * 40 bits student id, 8+8+8 bits for grades
 * grade: 1-100 points
 */
class Record final {
public:
    using data_t = uint64_t;
    using BytesArray = std::array<uint8_t, sizeof(data_t)>;
    static constexpr auto const GRADE_MAX = 100u;
    static constexpr auto const GRADE_MIN = 0u;

    Record() = delete;
    Record(Record const &) = default;
    Record(Record &&) = default;
    Record &operator=(Record const &) = default;
    Record &operator=(Record &&) = default;

    explicit Record(data_t data);
    explicit Record(std::string const &string);
    Record(int grade1, int grade2, int grade3);
    ~Record() = default;

    auto update(Record const &rec) -> void;

    auto get_student_id() const -> uint64_t;
    auto get_grade(int gradeNumber) const -> uint8_t;
    auto to_bytes() const -> BytesArray ;
    friend auto operator<<(std::ostream &os, const Record &record) -> std::ostream &;
    friend auto operator<<(std::stringstream &s, const Record &record) -> std::stringstream &;

    static auto Random() -> Record;
    auto operator<(Record const &rhs) const -> bool { return get_student_id() < rhs.get_student_id(); }
    auto operator>(Record const &rhs) const -> bool { return get_student_id() > rhs.get_student_id(); }
    auto operator<=(Record const &rhs) const -> bool { return get_student_id() <= rhs.get_student_id(); }
    auto operator>=(Record const &rhs) const -> bool { return get_student_id() >= rhs.get_student_id(); }

private:
    Record(uint64_t student_id, int grade1, int grade2, int grade3);


    data_t data;

    inline static uint64_t record_id_counter = 0;
    inline static std::random_device rd{};
    inline static std::mt19937_64 gen = std::mt19937_64{rd()};
    inline static std::uniform_int_distribution<> uid{GRADE_MIN, GRADE_MAX};
};

#endif //SBD_1_RECORD_HH