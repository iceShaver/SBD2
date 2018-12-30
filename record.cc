//
// Created by kamil on 19.10.18.
//

#include "record.hh"
#include "tools.hh"
#include <exception>
#include <stdexcept>
#include <string>
#include <cmath>
#include <iomanip>
#include <boost/algorithm/string.hpp>


Record::Record(data_t data) : data(data) {}

Record::Record(std::string const &string) {
    std::vector<std::string> tokens;
    boost::split(tokens, string, boost::is_any_of(" "));
    if (tokens.size() != 3)
        throw std::invalid_argument("Invalid data for creating Record");
    auto grade1 = std::stoi(tokens[0]);
    auto grade2 = std::stoi(tokens[1]);
    auto grade3 = std::stoi(tokens[2]);
    *this = std::move(Record(grade1, grade2, grade3));
}


Record::Record(uint64_t student_id, int grade1, int grade2, int grade3) {
    if (student_id >= std::pow(2, 40))
        throw std::invalid_argument("student id over " + std::to_string(std::pow(2, 40)));
    if (grade1 > GRADE_MAX || grade2 > GRADE_MAX || grade3 > GRADE_MAX ||
        grade1 < GRADE_MIN || grade2 < GRADE_MIN || grade3 < GRADE_MIN)
        throw std::invalid_argument(
                "invalid grade used to instatiate record. Must be <"
                + std::to_string(GRADE_MIN) + ", " + std::to_string(GRADE_MAX) + ">");
    data = grade3 | grade2 << 8 | grade1 << 16 | student_id << 24;
}


Record::Record(int grade1, int grade2, int grade3) : Record(record_id_counter++, grade1, grade2, grade3) {
    Tools::debug([] { std::clog << "Records constr called\n"; }, 4);
}


auto Record::get_grade(int gradeNumber) const -> uint8_t {
    if (gradeNumber <= GRADES_NUMBER && gradeNumber > 0)
        return static_cast<uint8_t>(data >> (3 - gradeNumber) * 8);
    throw std::invalid_argument("gradeNumber has to be <1, 3>");
}


auto Record::to_bytes() const -> BytesArray {
    auto result = std::array<uint8_t, sizeof(data_t)>();
    *reinterpret_cast<data_t *>(result.data()) = data;
    return result;
}


auto operator<<(std::ostream &os, const Record &record) -> std::ostream & {
    auto col_width = 3;
    return os << record.get_student_id() << ' '
              << std::setw(col_width) << +record.get_grade(1) << ' '
              << std::setw(col_width) << +record.get_grade(2) << ' '
              << std::setw(col_width) << +record.get_grade(3);
}


auto Record::get_student_id() const -> uint64_t {
    return data >> 24;
}


auto Record::Random() -> Record {
    return Record{static_cast<uint8_t>(uid(gen)), static_cast<uint8_t>(uid(gen)), static_cast<uint8_t>(uid(gen))};
}


auto Record::update(Record const &rec) -> void{
    auto oldId = this->get_student_id();
    *this = Record(this->get_student_id(), rec.get_grade(1), rec.get_grade(2), rec.get_grade(3));
}


auto operator<<(std::stringstream &s, const Record &record) -> std::stringstream &{
    s << record.get_student_id() + record.get_grade(1)
      << +record.get_grade(2)
      << +record.get_grade(3);
    return s;
}



