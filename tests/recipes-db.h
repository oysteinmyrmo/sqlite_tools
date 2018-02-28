#pragma once
#include <vector>
#include <sqlite_tools.h>
#include <json_tools/json_tools.h>

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
        
        JT_STRUCT(
            JT_MEMBER(id),
            JT_MEMBER(name),
            JT_MEMBER(cooking_time),
            JT_MEMBER(portions),
            JT_MEMBER(portions_unit),
            JT_MEMBER(description),
            JT_MEMBER(favorite)
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
        
        JT_STRUCT(
            JT_MEMBER(recipe_id),
            JT_MEMBER(ingredient_id)
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

        JT_STRUCT(
            JT_MEMBER(id),
            JT_MEMBER(name),
            JT_MEMBER(description),
            JT_MEMBER(vegetarian),
            JT_MEMBER(vegan)
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

        JT_STRUCT(
            JT_MEMBER(allergen_id),
            JT_MEMBER(ingredient_id)
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

        JT_STRUCT(
            JT_MEMBER(id),
            JT_MEMBER(name),
            JT_MEMBER(description)
        );

        SQLT_TABLE(allergens,
            SQLT_COLUMN_PRIMARY_KEY(id),
            SQLT_COLUMN(name),
            SQLT_COLUMN(description)
        );
    };

    SQLT_DATABASE_WITH_NAME(recipes_db, "asd",
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

        JT_STRUCT(
            JT_MEMBER(recipes),
            JT_MEMBER(ingredient_in_recipe),
            JT_MEMBER(ingredients),
            JT_MEMBER(allergen_in_ingredient),
            JT_MEMBER(allergens)
        );
    };
};

