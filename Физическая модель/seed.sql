-- Очистка таблиц перед наполнением
TRUNCATE TABLE Driver_Transportations, Transportations, Routes, Drivers, User_Roles, Users, Roles RESTART IDENTITY CASCADE;

-- 1. Заполнение ролей (Roles)
INSERT INTO Roles (role_name) VALUES
('Администратор'),
('Диспетчер'),
('Водитель'),
('Механик');

-- 2. Заполнение пользователей (Users)
-- Пароли соответствуют регулярному выражению: длина >= 8, содержат цифру, заглавную букву и спецсимвол
INSERT INTO Users (login, password) VALUES
('admin', 'Admin123!'),
('dispatcher', 'Disp123!'),
('ivanov_i', 'Ivanov123#'),
('petrov_p', 'Petrov123$'),
('sidorov_s', 'Sidorov123%'),
('kozlov_a', 'Kozlov123^'),
('mechanic', 'Mech123!');

-- 3. Связывание пользователей с ролями (User_Roles)
INSERT INTO User_Roles (id_user, id_role) VALUES
(1, 1), -- admin -> Администратор
(2, 2), -- dispatcher -> Диспетчер
(3, 3), -- ivanov_i -> Водитель
(3, 4), -- ivanov_i -> Механик (Водитель Иванов также является Механиком)
(4, 3), -- petrov_p -> Водитель
(5, 3), -- sidorov_s -> Водитель
(6, 3), -- kozlov_a -> Водитель
(7, 4); -- mechanic -> Механик

-- 4. Заполнение водителей (Drivers)
INSERT INTO Drivers (id_user, last_name, first_name, middle_name, experience) VALUES
(3, 'Иванов', 'Иван', 'Иванович', 1),    -- Стаж 1 год (коэффициент оплаты 1.0)
(4, 'Петров', 'Петр', 'Петрович', 3),    -- Стаж 3 года (коэффициент оплаты 1.15)
(5, 'Сидоров', 'Сидор', 'Сидорович', 7),  -- Стаж 7 лет (коэффициент оплаты 1.30)
(6, 'Козлов', 'Алексей', 'Михайлович', 12); -- Стаж 12 лет (коэффициент оплаты 1.50)

-- 5. Заполнение маршрутов (Routes)
INSERT INTO Routes (route_name, distance, base_payment) VALUES
('Москва - Санкт-Петербург', 700.00, 15000.00),
('Москва - Казань', 820.00, 18000.00),
('Новосибирск - Екатеринбург', 1600.00, 35000.00),
('Краснодар - Сочи', 290.00, 8000.00);

-- 6. Заполнение перевозок (Transportations)
INSERT INTO Transportations (id_route, departure_date, arrival_date) VALUES
(1, '2026-06-01 08:00:00', '2026-06-03 12:00:00'), -- Рейс 1 (завершен)
(2, '2026-06-05 09:00:00', '2026-06-07 15:00:00'), -- Рейс 2 (завершен)
(3, '2026-06-10 06:00:00', '2026-06-14 18:00:00'), -- Рейс 3 (завершен)
(4, '2026-06-20 10:00:00', NULL);                  -- Рейс 4 (в пути)

-- 7. Назначение водителей на рейсы (Driver_Transportations)
-- Оплата оставляется NULL при вставке, чтобы сработал триггер автоматического расчета
-- 7.1. Рейс 1: Назначен Иванов (стаж 1 год, базовый тариф 15000.00)
INSERT INTO Driver_Transportations (id_driver, id_transportation, payment, bonus_amount, bonus_reason) VALUES
(1, 1, NULL, 0, NULL); -- Ожидаемая оплата: 15000.00 * 1.0 = 15000.00

-- 7.2. Рейс 2: Назначен Петров (стаж 3 года, базовый тариф 18000.00) + премия
INSERT INTO Driver_Transportations (id_driver, id_transportation, payment, bonus_amount, bonus_reason) VALUES
(2, 2, NULL, 3000.00, 'Срочная доставка скоропортящегося груза'); -- Ожидаемая оплата: 18000.00 * 1.15 = 20700.00

-- 7.3. Рейс 3: Назначены два водителя (Сидоров стаж 7 лет, Козлов стаж 12 лет, базовая 35000.00)
INSERT INTO Driver_Transportations (id_driver, id_transportation, payment, bonus_amount, bonus_reason) VALUES
(3, 3, NULL, 0, NULL), -- Ожидаемая оплата: 35000.00 * 1.30 = 45500.00
(4, 3, NULL, 5000.00, 'Сложные дорожные условия в горной местности'); -- Ожидаемая оплата: 35000.00 * 1.50 = 52500.00

-- 7.4. Рейс 4: Назначен Иванов (рейс в пути, базовая 8000.00)
INSERT INTO Driver_Transportations (id_driver, id_transportation, payment, bonus_amount, bonus_reason) VALUES
(1, 4, NULL, 0, NULL); -- Ожидаемая оплата: 8000.00 * 1.0 = 8000.00
