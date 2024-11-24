CREATE TABLE department (id INT PRIMARY KEY, department_name VARCHAR(100) NOT NULL);
CREATE TABLE teacher (id INT PRIMARY KEY,name VARCHAR(100) NOT NULL,address VARCHAR(255),salary DECIMAL(10, 2),department_id INT,FOREIGN KEY (department_id) REFERENCES department(id));

INSERT INTO department (id, department_name) VALUES(1, 'Mathematics'),(2, 'Physics'),(3, 'Computer Science'),(4, 'Biology');

INSERT INTO teacher (id, name, address, salary, department_id) VALUES (1, 'Alice Johnson', '123 Maple Street', 8.50, 1), (2, 'Bob Smith', '456 Oak Avenue', 7.00, 2), (3, 'Carol White', '789 Pine Road', 9.20, 3), (4, 'David Black', '101 Birch Lane', 6.80, 1), (5, 'Eve Brown', '202 Elm Street', 10.00, NULL);

SELECT teacher.name AS teacher_name,teacher.salary,department.department_name FROM teacher LEFT JOIN department ON teacher.department_id = department.id WHERE teacher.salary > 7.00 ORDER BY teacher.salary DESC;
