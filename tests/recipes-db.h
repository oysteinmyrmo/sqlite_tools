#pragma once
#include <vector>
#include <sqlite_tools.h>
#include <json_struct/json_struct.h>

struct recipes_db
{
    struct recipes
    {
        int id;
        std::string name;
        int cooking_time;
        double portions;
        std::string portions_unit;
        SQLT::Nullable<std::string> description;
        SQLT::Nullable<int> favorite;

        JS_OBJECT(
            JS_MEMBER(id),
            JS_MEMBER(name),
            JS_MEMBER(cooking_time),
            JS_MEMBER(portions),
            JS_MEMBER(portions_unit),
            JS_MEMBER(description),
            JS_MEMBER(favorite)
        );

        SQLT_TABLE(recipes,
            SQLT_COLUMN_PRIMARY_KEY(id),
            SQLT_COLUMN(name),
            SQLT_COLUMN(cooking_time),
            SQLT_COLUMN(portions),
            SQLT_COLUMN(portions_unit),
            SQLT_COLUMN(description),
            SQLT_COLUMN_DEFAULT(favorite, 0)
        );
    };

    struct ingredient_in_recipe
    {
        int recipe_id;
        int ingredient_id;

        JS_OBJECT(
            JS_MEMBER(recipe_id),
            JS_MEMBER(ingredient_id)
        );

        SQLT_TABLE(ingredient_in_recipe,
            SQLT_COLUMN_PRIMARY_KEY(recipe_id),
            SQLT_COLUMN_PRIMARY_KEY(ingredient_id)
        );
    };

    struct ingredients
    {
        int id;
        std::string name;
        SQLT::Nullable<std::string> description;
        int vegetarian;
        int vegan;

        JS_OBJECT(
            JS_MEMBER(id),
            JS_MEMBER(name),
            JS_MEMBER(description),
            JS_MEMBER(vegetarian),
            JS_MEMBER(vegan)
        );

        SQLT_TABLE(ingredients,
            SQLT_COLUMN_PRIMARY_KEY(id),
            SQLT_COLUMN(name),
            SQLT_COLUMN(description),
            SQLT_COLUMN(vegetarian),
            SQLT_COLUMN(vegan)
        );
    };

    struct allergen_in_ingredient
    {
        int allergen_id;
        int ingredient_id;

        JS_OBJECT(
            JS_MEMBER(allergen_id),
            JS_MEMBER(ingredient_id)
        );

        SQLT_TABLE(allergen_in_ingredient,
            SQLT_COLUMN_PRIMARY_KEY(allergen_id),
            SQLT_COLUMN_PRIMARY_KEY(ingredient_id)
        );
    };

    struct allergens
    {
        int id;
        std::string name;
        SQLT::Nullable<std::string> description;

        JS_OBJECT(
            JS_MEMBER(id),
            JS_MEMBER(name),
            JS_MEMBER(description)
        );

        SQLT_TABLE(allergens,
            SQLT_COLUMN_PRIMARY_KEY(id),
            SQLT_COLUMN(name),
            SQLT_COLUMN(description)
        );
    };

    SQLT_DATABASE_WITH_NAME(recipes_db, "recipes_db.sqlite",
        SQLT_DATABASE_TABLE(recipes),
        SQLT_DATABASE_TABLE(ingredient_in_recipe),
        SQLT_DATABASE_TABLE(ingredients),
        SQLT_DATABASE_TABLE(allergen_in_ingredient),
        SQLT_DATABASE_TABLE(allergens)
    );

    struct DatabaseContainer
    {
        std::vector<recipes> recipes;
        std::vector<ingredient_in_recipe> ingredient_in_recipe;
        std::vector<ingredients> ingredients;
        std::vector<allergen_in_ingredient> allergen_in_ingredient;
        std::vector<allergens> allergens;

        JS_OBJECT(
            JS_MEMBER(recipes),
            JS_MEMBER(ingredient_in_recipe),
            JS_MEMBER(ingredients),
            JS_MEMBER(allergen_in_ingredient),
            JS_MEMBER(allergens)
        );
    };

    struct AllergenNames
    {
        decltype(allergens::name) name;

        SQLT_QUERY_RESULT_STRUCT(AllergenNames,
            SQLT_QUERY_RESULT_MEMBER(name)
        );
    };

    struct AllergensInIngredient
    {
        decltype(ingredients::id) ingredient_id;
        decltype(ingredients::name) ingredient_name;
        decltype(allergens::name) allergen_name;

        SQLT_QUERY_RESULT_STRUCT(AllergensInIngredient,
            SQLT_QUERY_RESULT_MEMBER(ingredient_id),
            SQLT_QUERY_RESULT_MEMBER(ingredient_name),
            SQLT_QUERY_RESULT_MEMBER(allergen_name)
        );
    };

    struct AllergensInRecipe
    {
        decltype(recipes::id) recipe_id;
        decltype(recipes::name) recipe_name;
        decltype(ingredients::id) ingredient_id;
        decltype(ingredients::name) ingredient_name;
        decltype(allergens::name) allergen_name;

        // Note: Intentionally different order of the members for testing such situations.
        SQLT_QUERY_RESULT_STRUCT(AllergensInIngredient,
            SQLT_QUERY_RESULT_MEMBER(allergen_name),
            SQLT_QUERY_RESULT_MEMBER(ingredient_name),
            SQLT_QUERY_RESULT_MEMBER(recipe_name),
            SQLT_QUERY_RESULT_MEMBER(recipe_id),
            SQLT_QUERY_RESULT_MEMBER(ingredient_id)
        );
    };
};
