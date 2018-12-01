//
// Created by kamil on 19.10.18.
//

#include "record.hh"
#include <exception>
#include <stdexcept>
#include <string>
#include <cmath>
#include <iomanip>

uint8_t Record::get_grade(int gradeNumber) const {
    if (gradeNumber <= GRADES_NUMBER && gradeNumber > 0)
        return static_cast<uint8_t>(data >> (3 - gradeNumber) * 8);
    throw std::invalid_argument("gradeNumber has to be <1, 3>");
}

Record::Record(data_t data) : data(data) { }

std::array<uint8_t, sizeof(Record::data_t)> Record::to_bytes() const {
    auto result = std::array<uint8_t, sizeof(data_t)>();
    *reinterpret_cast<data_t *>(result.data()) = data;
    return result;
}

Record::Record(uint64_t student_id, uint8_t grade1, uint8_t grade2, uint8_t grade3) {
    if (student_id >= std::pow(2, 40))
        throw std::invalid_argument("student id over " + std::to_string(std::pow(2, 40)));
    if (grade1 > GRADE_MAX || grade2 > GRADE_MAX || grade3 > GRADE_MAX ||
        grade1 < GRADE_MIN || grade2 < GRADE_MIN || grade3 < GRADE_MIN)
        throw std::invalid_argument("invalid grade used to instatiate record");
    data = grade3 | grade2 << 8 | grade1 << 16 | student_id << 24;
}

Record::Record(uint8_t grade1, uint8_t grade2, uint8_t grade3) : Record(record_id_counter++, grade1, grade2, grade3) {}

std::ostream &operator<<(std::ostream &os, const Record &record) {
    auto col_width = 3;
    return os << record.get_student_id()
              << std::setw(col_width) << +record.get_grade(1)
              << std::setw(col_width) << +record.get_grade(2)
              << std::setw(col_width) << +record.get_grade(3);
}

uint64_t Record::get_student_id() const { return data >> 24; }

Record Record::random() {
    return Record{static_cast<uint8_t>(uid(gen)), static_cast<uint8_t>(uid(gen)), static_cast<uint8_t>(uid(gen))};
}