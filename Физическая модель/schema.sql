-- Удаление существующих представлений (если они есть)
DROP VIEW IF EXISTS Transportation_Summary;
DROP VIEW IF EXISTS Salary_Statement;

-- Удаление существующих таблиц (каскадно)
DROP TABLE IF EXISTS Driver_Transportations CASCADE;
DROP TABLE IF EXISTS Transportations CASCADE;
DROP TABLE IF EXISTS Routes CASCADE;
DROP TABLE IF EXISTS Drivers CASCADE;
DROP TABLE IF EXISTS User_Roles CASCADE;
DROP TABLE IF EXISTS Users CASCADE;
DROP TABLE IF EXISTS Roles CASCADE;

-- ТАБЛИЦЫ

-- Таблица: Roles (Роли)
CREATE TABLE Roles (
    id_role SERIAL PRIMARY KEY,
    role_name VARCHAR(50) UNIQUE NOT NULL
);

COMMENT ON TABLE Roles IS 'Справочник ролей пользователей в системе';
COMMENT ON COLUMN Roles.id_role IS 'Уникальный идентификатор роли';
COMMENT ON COLUMN Roles.role_name IS 'Наименование роли (Администратор, Диспетчер, Водитель, Механик)';

-- Таблица: Users (Пользователи)
CREATE TABLE Users (
    id_user SERIAL PRIMARY KEY,
    login VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    -- Ограничение целостности: сложность пароля
    -- Длина >= 8, минимум одна цифра, одна заглавная буква и один специальный символ
    CONSTRAINT chk_password_strength CHECK (
        char_length(password) >= 8 AND
        password ~ '[0-9]' AND
        password ~ '[A-ZА-Я]' AND
        password ~ '[^a-zA-Z0-9а-яА-Я\s]'
    )
);

COMMENT ON TABLE Users IS 'Учетные записи пользователей информационной системы';
COMMENT ON COLUMN Users.id_user IS 'Уникальный идентификатор пользователя';
COMMENT ON COLUMN Users.login IS 'Уникальное имя пользователя для авторизации';
COMMENT ON COLUMN Users.password IS 'Пароль пользователя (подлежит хэшированию в приложении или проверке регулярным выражением)';

-- Таблица: User_Roles (Роли_Пользователи)
CREATE TABLE User_Roles (
    id_user INT NOT NULL REFERENCES Users(id_user) ON DELETE CASCADE,
    id_role INT NOT NULL REFERENCES Roles(id_role) ON DELETE CASCADE,
    PRIMARY KEY (id_user, id_role)
);

COMMENT ON TABLE User_Roles IS 'Связующая таблица для ролей пользователей';

-- Таблица: Drivers (Водители)
CREATE TABLE Drivers (
    id_driver SERIAL PRIMARY KEY,
    id_user INT UNIQUE REFERENCES Users(id_user) ON DELETE SET NULL,
    last_name VARCHAR(50) NOT NULL,
    first_name VARCHAR(50) NOT NULL,
    middle_name VARCHAR(50),
    experience INT NOT NULL DEFAULT 0 CHECK (experience >= 0)
);

COMMENT ON TABLE Drivers IS 'Информация о водителях компании';
COMMENT ON COLUMN Drivers.id_driver IS 'Уникальный идентификатор водителя';
COMMENT ON COLUMN Drivers.id_user IS 'Ссылка на учетную запись пользователя (1-к-1)';
COMMENT ON COLUMN Drivers.last_name IS 'Фамилия водителя';
COMMENT ON COLUMN Drivers.first_name IS 'Имя водителя';
COMMENT ON COLUMN Drivers.middle_name IS 'Отчество водителя (при наличии)';
COMMENT ON COLUMN Drivers.experience IS 'Стаж работы водителя в годах';

-- Таблица: Routes (Маршруты)
CREATE TABLE Routes (
    id_route SERIAL PRIMARY KEY,
    route_name VARCHAR(100) UNIQUE NOT NULL,
    distance NUMERIC(10, 2) NOT NULL CHECK (distance > 0),
    base_payment NUMERIC(12, 2) NOT NULL CHECK (base_payment >= 0)
);

COMMENT ON TABLE Routes IS 'Справочник маршрутов грузоперевозок';
COMMENT ON COLUMN Routes.id_route IS 'Уникальный идентификатор маршрута';
COMMENT ON COLUMN Routes.route_name IS 'Уникальное название маршрута';
COMMENT ON COLUMN Routes.distance IS 'Расстояние маршрута в километрах';
COMMENT ON COLUMN Routes.base_payment IS 'Базовая ставка оплаты водителю за данный маршрут';

-- Таблица: Transportations (Перевозки)
CREATE TABLE Transportations (
    id_transportation SERIAL PRIMARY KEY,
    id_route INT NOT NULL REFERENCES Routes(id_route) ON DELETE RESTRICT,
    departure_date TIMESTAMP NOT NULL,
    arrival_date TIMESTAMP,
    -- Дата прибытия должна быть позже или равна дате отправления
    CONSTRAINT chk_dates CHECK (arrival_date IS NULL OR arrival_date >= departure_date)
);

COMMENT ON TABLE Transportations IS 'Информация о выполненных и текущих перевозках';
COMMENT ON COLUMN Transportations.id_transportation IS 'Уникальный идентификатор перевозки';
COMMENT ON COLUMN Transportations.id_route IS 'Ссылка на маршрут перевозки';
COMMENT ON COLUMN Transportations.departure_date IS 'Дата и время отправления рейса';
COMMENT ON COLUMN Transportations.arrival_date IS 'Дата и время прибытия рейса (NULL, если рейс в пути)';

-- Таблица: Driver_Transportations (Водители_Перевозки)
CREATE TABLE Driver_Transportations (
    id_driver INT NOT NULL REFERENCES Drivers(id_driver) ON DELETE CASCADE,
    id_transportation INT NOT NULL REFERENCES Transportations(id_transportation) ON DELETE CASCADE,
    payment NUMERIC(12, 2) CHECK (payment >= 0),
    bonus_amount NUMERIC(12, 2) DEFAULT 0 CHECK (bonus_amount >= 0),
    bonus_reason VARCHAR(255),
    PRIMARY KEY (id_driver, id_transportation)
);

COMMENT ON TABLE Driver_Transportations IS 'Связующая таблица: назначение водителей на рейсы и их заработная плата';
COMMENT ON COLUMN Driver_Transportations.id_driver IS 'Ссылка на водителя';
COMMENT ON COLUMN Driver_Transportations.id_transportation IS 'Ссылка на перевозку';
COMMENT ON COLUMN Driver_Transportations.payment IS 'Фактическая оплата водителю за рейс (с учетом стажа)';
COMMENT ON COLUMN Driver_Transportations.bonus_amount IS 'Размер премии за рейс';
COMMENT ON COLUMN Driver_Transportations.bonus_reason IS 'Причина начисления премии';

-- ИНДЕКСЫ

-- Индекс для быстрого поиска пользователя по логину при авторизации
CREATE INDEX idx_Users_login ON Users(login);

-- Индекс для фильтрации перевозок по датам
CREATE INDEX idx_Transportations_dates ON Transportations(departure_date, arrival_date);

-- Индекс для быстрого нахождения перевозок по маршруту
CREATE INDEX idx_Transportations_route ON Transportations(id_route);

-- Индекс по id_transportation для ускорения поиска водителей по перевозке
CREATE INDEX idx_Driver_trans_link ON Driver_Transportations(id_transportation);

-- ТРИГГЕРЫ

-- 4.1 Триггер: Проверка максимального числа водителей на рейс (не более двух)
CREATE OR REPLACE FUNCTION fn_check_drivers_count()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT COUNT(*) FROM Driver_Transportations WHERE id_transportation = NEW.id_transportation) >= 2 THEN
        RAISE EXCEPTION 'Превышен лимит водителей! На одну перевозку может быть назначено не более двух водителей.';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_check_drivers_count
BEFORE INSERT ON Driver_Transportations
FOR EACH ROW
EXECUTE FUNCTION fn_check_drivers_count();


-- 4.2 Trigger: Автоматический расчет оплаты труда водителя с учетом его стажа
-- Формула: payment = base_payment_маршрута * коэффициент_стажа
-- Коэффициенты:
--   стаж < 2 лет = 1.0 (без надбавки)
--   стаж от 2 до 5 лет = 1.15 (+15%)
--   стаж от 5 до 10 лет = 1.30 (+30%)
--   стаж от 10 лет = 1.50 (+50%)
CREATE OR REPLACE FUNCTION fn_calculate_driver_payment()
RETURNS TRIGGER AS $$
DECLARE
    v_base_pay NUMERIC(12, 2);
    v_experience INT;
    v_coef NUMERIC(3, 2);
BEGIN
    -- Расчет производится только в том случае, если оплата не была введена вручную
    IF NEW.payment IS NULL THEN
        -- 1. Получаем базовую оплату из маршрута, связанного с перевозкой
        SELECT r.base_payment INTO v_base_pay
        FROM Transportations t
        JOIN Routes r ON t.id_route = r.id_route
        WHERE t.id_transportation = NEW.id_transportation;
        
        -- 2. Получаем стаж водителя
        SELECT d.experience INTO v_experience
        FROM Drivers d
        WHERE d.id_driver = NEW.id_driver;
        
        -- 3. Вычисляем коэффициент стажа
        IF v_experience < 2 THEN
            v_coef := 1.00;
        ELSIF v_experience < 5 THEN
            v_coef := 1.15;
        ELSIF v_experience < 10 THEN
            v_coef := 1.30;
        ELSE
            v_coef := 1.50;
        END IF;
        
        -- 4. Устанавливаем итоговую оплату
        NEW.payment := v_base_pay * v_coef;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_calculate_driver_payment
BEFORE INSERT OR UPDATE OF payment, id_driver, id_transportation ON Driver_Transportations
FOR EACH ROW
EXECUTE FUNCTION fn_calculate_driver_payment();

-- ПРЕДСТАВЛЕНИЯ

-- 5.1 Представление: Salary_Statement (Зарплатная ведомость)
CREATE VIEW Salary_Statement AS
SELECT 
    d.id_driver,
    CONCAT(d.last_name, ' ', d.first_name, ' ', COALESCE(d.middle_name, '')) AS driver_full_name,
    d.experience AS experience_years,
    COUNT(dt.id_transportation) FILTER (WHERE t.arrival_date IS NOT NULL) AS completed_trips,
    COUNT(dt.id_transportation) FILTER (WHERE t.arrival_date IS NULL) AS trips_in_progress,
    COALESCE(SUM(dt.payment), 0) AS total_payment,
    COALESCE(SUM(dt.bonus_amount), 0) AS total_bonuses,
    COALESCE(SUM(dt.payment + dt.bonus_amount), 0) AS total_earned
FROM Drivers d
LEFT JOIN Driver_Transportations dt ON d.id_driver = dt.id_driver
LEFT JOIN Transportations t ON dt.id_transportation = t.id_transportation
GROUP BY d.id_driver, d.last_name, d.first_name, d.middle_name, d.experience;

-- 5.2 Представление: Transportation_Summary (Сводный отчет по перевозкам)
CREATE VIEW Transportation_Summary AS
SELECT 
    t.id_transportation,
    r.route_name,
    r.distance AS distance_km,
    t.departure_date,
    t.arrival_date,
    COALESCE(
        STRING_AGG(
            CONCAT(d.last_name, ' ', SUBSTRING(d.first_name, 1, 1), '.', COALESCE(CONCAT(SUBSTRING(d.middle_name, 1, 1), '.'), '')), 
            ', '
        ), 
        'Not assigned'
    ) AS drivers,
    COALESCE(SUM(dt.payment), 0) AS total_drivers_payment,
    COALESCE(SUM(dt.bonus_amount), 0) AS total_drivers_bonuses,
    ROUND((r.base_payment * 1.8 + COALESCE(SUM(dt.payment + dt.bonus_amount), 0)), 2) AS total_transportation_cost
FROM Transportations t
JOIN Routes r ON t.id_route = r.id_route
LEFT JOIN Driver_Transportations dt ON t.id_transportation = dt.id_transportation
LEFT JOIN Drivers d ON dt.id_driver = d.id_driver
GROUP BY t.id_transportation, r.route_name, r.distance, r.base_payment, t.departure_date, t.arrival_date;

-- РОЛИ

-- Создание ролей базы данных (если они не существуют)
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'admin_role') THEN
        CREATE ROLE admin_role;
    END IF;
    IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'dispatcher_role') THEN
        CREATE ROLE dispatcher_role;
    END IF;
    IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'driver_role') THEN
        CREATE ROLE driver_role;
    END IF;
    IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'mechanic_role') THEN
        CREATE ROLE mechanic_role;
    END IF;
END $$;

-- -- Сброс всех прав доступа по умолчанию с таблиц схемы public для роли PUBLIC (все новые пользователи)
-- REVOKE ALL ON ALL TABLES IN SCHEMA public FROM PUBLIC;
-- REVOKE ALL ON ALL SEQUENCES IN SCHEMA public FROM PUBLIC;

-- 6.1. Права роли: admin_role (Администратор - Полный доступ)
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO admin_role;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO admin_role;

GRANT SELECT, INSERT, UPDATE ON TABLE Transportations TO dispatcher_role;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE Driver_Transportations TO dispatcher_role;
GRANT SELECT, INSERT, UPDATE ON TABLE Drivers, Routes TO dispatcher_role;
GRANT SELECT ON TABLE Users, Roles, User_Roles TO dispatcher_role;
-- Также права на использование автоинкрементных последовательностей для INSERT
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO dispatcher_role;

-- 6.3. Права роли: driver_role (Водитель - Рядовой пользователь)
-- Может только просматривать информацию о водителях, маршрутах и перевозках
GRANT SELECT ON TABLE Drivers, Routes, Transportations, Driver_Transportations TO driver_role;
GRANT SELECT ON TABLE Salary_Statement, Transportation_Summary TO driver_role;

-- 6.4. Права роли: mechanic_role (Механик - Технический наблюдатель)
-- Доступ только на просмотр перевозок и водителей
GRANT SELECT ON TABLE Transportations, Drivers, Routes TO mechanic_role;
GRANT SELECT ON TABLE Transportation_Summary TO mechanic_role;
